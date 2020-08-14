#include <iostream>
#include "spdlog/logger.h"
#include "Lobby.hpp"
#include "LocalGameBuilder.hpp"

#ifndef NDEBUG
#define STOP_SIGS SIGTERM, SIGHUP
#else
#define STOP_SIGS SIGINT, SIGTERM, SIGHUP
#endif

std::atomic_bool critical_stop { false };
std::atomic_bool closed { false };

void run(io::io_context& server, Lobby& lobby, spdlog::logger& logger) {
    while (!(critical_stop || closed)) {
        try {
            server.run_for(std::chrono::seconds { 3 });
        } catch (const std::exception& err) {
            server.stop();
            lobby.close(true);

            logger.critical(err.what());
            critical_stop = true;
        }
    }
}

int main(const int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Utilisation : <ip> <port>" << std::endl;
        return 1;
    }

    const std::string ip { argv[1] };
    ushort port;

    try {
        if (ip != "ipv4" && ip != "ipv6")
            throw std::logic_error { "Protocole IP inconnu" };

        port = std::stoi(std::string { argv[2] });
    } catch (const std::logic_error&) {
        std::cerr << "Utilisation : <ip> <port>" << std::endl;
        return 1;
    }

    spdlog::logger& logger { rboLogger("main") };

    logger.info("Lancement du serveur...");
    io::io_context server;

    try {
        const LocalGameBuilder game_builder {
            "game/game.json", "game/chkpts.json", "game/scenes.json", "game/instructions"
        };

        Lobby lobby {
            server,
            tcp::endpoint { ip == "ipv4" ? tcp::v4() : tcp::v6(), port },
            game_builder,
        };

        io::signal_set stop_handler { server, STOP_SIGS };
        stop_handler.async_wait([&server, &lobby, &logger](const ErrCode err, const int sig) {
            std::cout << "\b\b";

            if (err) {
                throw std::runtime_error {
                    "Arrêt impossible " + std::to_string(sig) + " : " + err.message()
                };
            }

            logger.debug("Signal d'arrêt {}", sig);
            server.stop();
            lobby.close();

            closed = true;
        });

        lobby.open();

        const auto run_server { std::bind(run, std::ref(server), std::ref(lobby), std::ref(logger)) };
        std::thread server_t1 { run_server };
        std::thread server_t2 { run_server };

        server_t1.join();
        server_t2.join();
    } catch (const InstructionLoadingError& err) {
        logger.critical(err.what());
        return 2;
    } catch (const GameLoadingError& err) {
        logger.critical(err.what());
        return 2;
    }

    if (critical_stop)
        return 3;

    logger.info("Arrêt du serveur.");
    return 0;
}
