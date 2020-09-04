#include <Rbo/Server/LobbyExecutor.hpp>

#include <spdlog/logger.h>
#include <Rbo/Server/Lobby.hpp>

namespace Rbo::Server {

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
        thread = std::thread { std::bind(&LobbyExecutor::runExecutor, this) };

    logger_.trace("{} threads lancés pour l'exécution du serveur.", threads_.size());

    for (std::thread& thread : threads_)
        thread.join();

    assert(error_ || closed_);
    return !error_;
}

void LobbyExecutor::stop() {
    closed_ = true;

    server_.stop();
    lobby_.close();
}

} // namespace Rbo::Server
