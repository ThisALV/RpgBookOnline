#include <Rbo/Server/LobbyExecutor.hpp>

#include <spdlog/logger.h>
#include <Rbo/Server/Lobby.hpp>

namespace Rbo::Server {

LobbyExecutor::LobbyExecutor(const std::size_t threads, io::io_context& server, Lobby& lobby, spdlog::logger& logger)
    : threads_ { threads },
      server_ { server },
      lobby_ { lobby },
      logger_ { logger },
      closed_ { false },
      error_ { false },
      stop_handler_done_ { false }
{
    assert(threads >= 2);
}

void LobbyExecutor::runExecutor() {
    while (!(error_ || closed_)) {
        try {
            server_.run_for(std::chrono::milliseconds { 500 });
        } catch (const std::exception& err) {
            server_.stop();
            lobby_.close(true);

            logger_.critical(err.what());
            error_ = true;
        }
    }
}

bool LobbyExecutor::start() {
    lobby_.open();

    for (std::thread& thread : threads_)
        thread = std::thread { [this]() { runExecutor(); } };

    logger_.info("{} given threads for server execution.", threads_.size());

    for (std::thread& thread : threads_)
        thread.join();

    while (!stop_handler_done_)
        std::this_thread::sleep_for(std::chrono::milliseconds { 1 });

    assert(error_ || closed_);
    return !error_;
}

void LobbyExecutor::stop() {
    closed_ = true;

    server_.stop();
    lobby_.close();

    stop_handler_done_ = true;
}

} // namespace Rbo::Server
