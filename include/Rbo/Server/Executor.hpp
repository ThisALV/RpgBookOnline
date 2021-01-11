#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include <Rbo/AsioCommon.hpp>

#include <mutex>
#include <thread>
#include <spdlog/logger.h>
#include <Rbo/GameBuilder.hpp>
#include <Rbo/Server/Lobby.hpp>

namespace Rbo::Server {

class Executor {
private:
    enum State {
        Running, Stopped, EventsLoopError, ServerError
    };

    spdlog::logger& logger_;
    io::io_context& server_;
    std::mutex lobby_mtx_;
    Lobby& lobby_;

    std::mutex session_mtx_;
    std::optional<Session> session_;

    std::atomic<State> state_;
    std::atomic_bool stop_handler_done_;

    void stop(const State stopped_reason, const std::string_view err_msg = "");
    void runEventsLoop();

    bool isRunning() { return state_ == Running; }
    bool hasError() { return state_ == EventsLoopError || state_ == ServerError; }

public:
    Executor(io::io_context& server, Lobby& lobby, spdlog::logger& logger);

    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    bool operator==(const Executor&) const = delete;

    template<typename ExecutorGameBuilder, typename ... BuilderArgs>
    bool start(BuilderArgs&& ... game_builder_args) {
        state_ = Running;
        std::thread events_loop { [this]() { runEventsLoop(); } };

        try {
            logger_.debug("<-- Running server on this thread.");
            while (isRunning()) {
                std::unique_lock lobby_lock { lobby_mtx_, std::defer_lock };

                lobby_lock.lock();
                if (lobby_.isIdle())
                    lobby_.open();
                lobby_lock.unlock();

                while (lobby_.isOpen() || lobby_.isStarting())
                    std::this_thread::sleep_for(std::chrono::milliseconds { 1 });

                try {
                    std::optional<ExecutorGameBuilder> game_builder;
                    if (lobby_.isPreparing()) {
                        const std::lock_guard session_lock { session_mtx_ };

                        game_builder.emplace(std::forward<BuilderArgs>(game_builder_args)...);
                        session_.emplace(*game_builder);
                    }

                    lobby_lock.lock();
                    if (lobby_.isPreparing())
                        lobby_.prepareSession(*session_);
                    lobby_lock.unlock();

                    const std::lock_guard session_lock { session_mtx_ };
                    session_.reset();
                } catch (const GameBuildingError& err) {
                    logger_.error(err.what());

                    lobby_lock.lock();
                    if (lobby_.isPreparing()) {
                        logger_.warn("Game building has failed, reopening lobby...");
                        lobby_.reset();
                    }
                    lobby_lock.unlock();

                    const std::lock_guard session_lock { session_mtx_ };
                    session_.reset();
                }
            }

            while (!stop_handler_done_)
                std::this_thread::sleep_for(std::chrono::milliseconds { 1 });
        } catch (const std::exception& err) {
            stop(ServerError, err.what());
        }

        assert(!isRunning());
        events_loop.join();

        return !hasError();
    }

    void stop() { stop(Stopped); }
};

} // namespace Rbo::Server

#endif // LOBBY_EXECUTOR_HPP
