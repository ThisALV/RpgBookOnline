#ifndef LOBBYEXECUTOR_HPP
#define LOBBYEXECUTOR_HPP

#include "AsioCommon.hpp"

namespace Rbo::Server {

class Lobby;

class LobbyExecutor {
private:
    std::vector<std::thread> threads_;

    io::io_context& server_;
    Lobby& lobby_;
    spdlog::logger& logger_;

    std::atomic_bool closed_;
    std::atomic_bool error_;

    void runExecutor();

public:
    LobbyExecutor(const std::size_t threads, io::io_context& server, Lobby& lobby,
                  spdlog::logger& logger)
        : threads_ { threads }, server_ { server }, lobby_ { lobby }, logger_ { logger },
          closed_ { false }, error_ { false }
    {
        assert(threads >= 2);
    }

    bool start();
    void stop();
};

} // namespace Rbo::Server

#endif // SERVER_HPP