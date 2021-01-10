#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include <Rbo/AsioCommon.hpp>

#include <mutex>
#include <Rbo/GameBuilder.hpp>
#include <Rbo/Server/Lobby.hpp>

namespace Rbo::Server {

class Executor {
private:
    enum State {
        Running, Stopped, EventsLoopError, ServerError
    };

    io::io_context& server_;
    std::mutex lobby_mtx_;
    Lobby& lobby_;
    std::mutex session_mtx_;
    SessionPtr session_;
    spdlog::logger& logger_;

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

    bool start(const GameBuilder& game_builder);
    void stop() { stop(Stopped); }
};

} // namespace Rbo::Server

#endif // LOBBY_EXECUTOR_HPP
