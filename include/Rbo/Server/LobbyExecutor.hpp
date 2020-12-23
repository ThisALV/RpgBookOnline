#ifndef LOBBYEXECUTOR_HPP
#define LOBBYEXECUTOR_HPP

#include <Rbo/AsioCommon.hpp>

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
    std::atomic_bool stop_handler_done_;

    void runExecutor();

public:
    LobbyExecutor(const std::size_t threads, io::io_context& server, Lobby& lobby, spdlog::logger& logger);

    LobbyExecutor(const LobbyExecutor&) = delete;
    LobbyExecutor& operator=(const LobbyExecutor&) = delete;

    bool operator==(const LobbyExecutor&) const = delete;

    bool start();
    void stop();
};

} // namespace Rbo::Server

#endif // SERVER_HPP
