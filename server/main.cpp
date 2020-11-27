#include <iostream>
#include <spdlog/logger.h>
#include <Rbo/Server/Lobby.hpp>
#include <Rbo/Server/LobbyExecutor.hpp>
#include <Rbo/Server/LocalGameBuilder.hpp>

#if defined(WIN32) || defined(_WIN32)

#include <Windows.h>

std::function<void(const Rbo::ErrCode, const int)> lobby_stop_handler;

BOOL WINAPI StopHandler(DWORD event) {
    switch (event) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
        lobby_stop_handler(Rbo::ErrCode {}, 15);
        return TRUE;
    case CTRL_CLOSE_EVENT:
        lobby_stop_handler(Rbo::ErrCode {}, 1);
        return TRUE;
    default:
        return FALSE;
    }
}

#else

#ifdef NDEBUG // En Debug, GDB interprète SIGINT pour mettre en pause le programme
#define STOP_SIGS SIGHUP, SIGINT, SIGTERM
#else
#define STOP_SIGS SIGHUP, SIGTERM
#endif

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

        const auto stop_handler = [&executor, &logger](const Rbo::ErrCode err, const int sig) {
            std::cout << "\b\b";

            if (err) {
                throw std::runtime_error { "Impossible shutdown " + std::to_string(sig) + " : " + err.message() };
            }

            logger.debug("Shutdown signal : {}", sig);
            executor.stop();
        };

#if defined(WIN32) || defined(_WIN32)
        lobby_stop_handler = stop_handler;

        if (!SetConsoleCtrlHandler(StopHandler, TRUE))
            logger.warn("Stop handler initialization failed");
#else
        Rbo::io::signal_set stop_sigs_handler { server, STOP_SIGS };
        stop_sigs_handler.async_wait(stop_handler);
#endif

        done = executor.start();
    } catch (const boost::system::system_error& err) {
        logger.critical(err.what());
        return 3;
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
