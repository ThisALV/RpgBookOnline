#include <Rbo/Server/Executor.hpp>

#include <thread>
#include <spdlog/logger.h>
#include <Rbo/Server/Lobby.hpp>

namespace Rbo::Server {

Executor::Executor(io::io_context& server, Lobby& lobby, spdlog::logger& logger)
    : server_ { server },
      lobby_ { lobby },
      logger_ { logger },
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

bool Executor::start(const GameBuilder& game_builder) {
    state_ = Running;
    std::thread events_loop { [this]() { runEventsLoop(); } };

    try {
        logger_.debug("<-- Running server on this thread.");
        while (isRunning()) {
            std::unique_lock session_lock { session_mtx_ };
            session_ = std::make_unique<Session>(game_builder);
            session_lock.unlock();

            std::unique_lock lobby_lock { lobby_mtx_, std::defer_lock };

            lobby_lock.lock();
            if (lobby_.isIdle())
                lobby_.open();
            lobby_lock.unlock();

            while (lobby_.isOpen() || lobby_.isStarting())
                std::this_thread::sleep_for( std::chrono::milliseconds { 1 } );

            lobby_lock.lock();
            if (lobby_.isPreparing())
                lobby_.prepareSession(session_);
            lobby_lock.unlock();
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
