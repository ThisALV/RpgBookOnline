#include <iostream>
#include <spdlog/logger.h>
#include <Rbo/Server/Lobby.hpp>
#include <Rbo/Server/LobbyExecutor.hpp>
#include <Rbo/Server/LocalGameBuilder.hpp>

#ifdef NDEBUG // En Debug, GDB interprête SIGINT
#define BASIC_STOP_SIGS SIGINT, SIGTERM
#else
#define BASIC_STOP_SIGS SIGTERM
#endif

#ifdef SIGHUP // Windows ne fournit pas SIGHUP
#define STOP_SIGS BASIC_STOP_SIGS, SIGHUP
#else
#define STOP_SIGS BASIC_STOP_SIGS
#endif

int main(const int argc, const char* argv[]) {
    const std::string usage { "Usage : <ip> <port> <threads> <prepare_delay (ms)>" };

    if (argc != 5) {
        std::cerr << usage << std::endl;
        return 1;
    }

    const std::string ip { argv[1] };
    ushort port;
    ulong threads;
    ulong prepare_delay;

    try {
        if (ip != "ipv4" && ip != "ipv6")
            throw std::logic_error { "Unknown IP protocol" };

        port = std::stoi(std::string { argv[2] });
        threads = std::stoul(std::string { argv[3] });
        prepare_delay = std::stoul(std::string { argv[4] });
    } catch (const std::logic_error&) {
        std::cerr << usage << std::endl;
        return 1;
    }

    spdlog::logger& logger { Rbo::rboLogger("main") };

    bool done;
    try {
        logger.info("Starting server...");

        const Rbo::Server::LocalGameBuilder game_builder {
            "game/game.json", "game/chkpts.json", "game/scenes.lua", "instructions"
        };

        Rbo::io::io_context server;
        Rbo::Server::Lobby lobby {
            server,
            Rbo::tcp::endpoint { ip == "ipv4" ? Rbo::tcp::v4() : Rbo::tcp::v6(), port },
            game_builder,
            prepare_delay
        };

        Rbo::Server::LobbyExecutor executor { threads, server, lobby, logger };

        Rbo::io::signal_set stop_handler { server, STOP_SIGS };
        stop_handler.async_wait([&executor, &logger](const Rbo::ErrCode err, const int sig) {
            std::cout << "\b\b";

            if (err) {
                throw std::runtime_error { "Impossible shutdown " + std::to_string(sig) + " : " + err.message() };
            }

            logger.debug("Shutdown signal : {}", sig);
            executor.stop();
        });

        done = executor.start();
    } catch (const Rbo::Server::ScriptLoadingError& err) {
        logger.critical(err.what());
        return 2;
    } catch (const Rbo::Server::GameLoadingError& err) {
        logger.critical(err.what());
        return 2;
    }

    if (!done)
        return 3;

    logger.info("Server stopped.");
    return 0;
}
