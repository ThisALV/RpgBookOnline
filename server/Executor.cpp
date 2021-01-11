#include <Rbo/Server/Executor.hpp>

#include <thread>
#include <spdlog/logger.h>
#include <Rbo/Server/Lobby.hpp>
#include <Rbo/Server/LocalGameBuilder.hpp>

namespace Rbo::Server {

Executor::Executor(io::io_context& server, Lobby& lobby, spdlog::logger& logger)
    : logger_ { logger },
      server_ { server },
      lobby_ { lobby },
      state_ { Stopped },
      stop_handler_done_ { false } {}

void Executor::stop(const State stopped_reason, const std::string_view err_msg) {
    assert(stopped_reason != Running);

    state_ = stopped_reason;
    server_.stop();

    lobby_.requestClosure();

    std::unique_lock session_lock { session_mtx_ };
    if (session_)
        session_->stop();
    session_lock.unlock();

    const bool is_reason_error { hasError() };

    std::unique_lock lobby_lock { lobby_mtx_ };
    lobby_.close(is_reason_error);
    lobby_lock.unlock();

    if (is_reason_error)
        logger_.critical(err_msg);

    stop_handler_done_ = true;
}

void Executor::runEventsLoop() {
    logger_.debug("<-- Running events loop on this thread.");

    while (isRunning()) {
        try {
            server_.run_for(std::chrono::milliseconds { 10 });
        } catch (const std::exception& err) {
            stop(EventsLoopError, err.what());
        }
    }
}

bool Executor::start(const GameBuilderGenerator& game_builder_generator) {
    state_ = Running;
    std::thread events_loop { [this]() { runEventsLoop(); } };

    GameBuilderPtr game_builder;
    try {
        logger_.debug("<-- Running server on this thread.");
        while (isRunning()) {
            std::unique_lock lobby_lock { lobby_mtx_, std::defer_lock };

            lobby_lock.lock();
            if (lobby_.isIdle())
                lobby_.open();
            lobby_lock.unlock();

            while (lobby_.isOpen() || lobby_.isStarting())
                std::this_thread::sleep_for( std::chrono::milliseconds { 1 } );

            try {
                if (lobby_.isPreparing()) {
                    const std::lock_guard session_lock{session_mtx_};

                    session_.reset(); // Le GameBuilder sur lequel session_ a une référence s'apprête à être détruit
                    game_builder = game_builder_generator();
                    session_ = std::make_unique<Session>(*game_builder);
                }

                lobby_lock.lock();
                if (lobby_.isPreparing())
                    lobby_.prepareSession(session_);
                lobby_lock.unlock();
            } catch (const GameBuildingError& err) {
                logger_.error(err.what());

                lobby_lock.lock();
                if (lobby_.isPreparing()) {
                    logger_.warn("Game building has failed, reopening lobby...");
                    lobby_.reset();
                }
                lobby_lock.unlock();
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

} // namespace Rbo::Server
