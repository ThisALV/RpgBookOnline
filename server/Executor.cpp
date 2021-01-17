#include <Rbo/Server/Executor.hpp>

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
            server_.run();
        } catch (const std::exception& err) {
            stop(EventsLoopError, err.what());
        }
    }
}

} // namespace Rbo::Server
