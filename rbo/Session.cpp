#include <Rbo/Session.hpp>

#include <iostream>
#include <numeric>
#include <spdlog/logger.h>
#include <spdlog/fmt/ostr.h>
#include <Rbo/SessionDataFactory.hpp>
#include <Rbo/GameBuilder.hpp>
#include <Rbo/Gameplay.hpp>
#include <Rbo/ReplyHandler.hpp>

namespace Rbo {

InvalidIDs::InvalidIDs(const std::vector<byte>& expected_ids, const ParticipantsValidity type) : std::logic_error { "Invalid IDs" }, expectedIDs { expected_ids }, errType { type } {
    assert(type != ParticipantsValidity::Ok);

    msg = "Expected IDs :";
    for (const byte expected : expectedIDs)
        msg += ' ' + std::to_string(expected) + ';';
}

const char* CancelledRequest::what() const noexcept {
    return "CancelledRequest";
}

void Session::logPlayerError(spdlog::logger& logger, const byte player, const std::string& err) {
    logger.error("Player {} : {}", player, err);
}

std::string Session::initStatMsg(const DiceFormula& init, const std::string& name, const int value) {
    std::string msg { name + " vaut " };
    if (init.dices == 0)
        msg += std::to_string(init.bonus);
    else
        msg += std::to_string(init.dices) + " dé(s) 6 + " + std::to_string(init.bonus) + " = " + std::to_string(value);

    return msg;
}

tcp::socket& Session::connection(const byte id) {
    assert(connections_.count(id) == 1);

    return connections_.at(id);
}

void Session::disconnect(const byte id, const bool crash) {
    if (!crash)
        connection(id).shutdown(tcp::socket::shutdown_both);

    removePlayer(id);

    if (!playersRemaining())
        throw NoPlayerRemaining {};

    if (leader() == id)
        switchLeader(players_.cbegin()->first);

    if (crash) {
        SessionDataFactory data_factory;
        data_factory.makeCrash(id);

        sendToAll(data_factory.dataWithLength());
    }
}

void Session::removePlayer(const byte id) {
    players_.erase(id);
    connections_.erase(id);
}

Session::Session(io::io_context& io, const GameBuilder& g_builder)
    : executor_ { io },
      logger_ { rboLogger("Session") },
      game_ { g_builder() },
      first_state_ { true },
      running_ { false },
      game_builer_ { g_builder } {}

void Session::end(Participants& participants) {
    SessionDataFactory stop_msg;
    stop_msg.makeData(DataType::Stop);

    sendToAll(stop_msg.dataWithLength());

    std::vector<byte> error_ids;
    for (auto& [id, participant] : participants) {
        if (connections_.count(id) == 1) {
            logger_.trace("Déplacement du socket du participant {}.", id);
            participant.socket = std::move(connection(id));
        } else {
            error_ids.push_back(id);
        }
    }

    for (const byte id : error_ids) {
        logger_.trace("Suppression du participant {}.", id);
        participants.erase(id);
    }

    stop();
};

void Session::start(std::map<byte, Particpant>& participants, const std::string& checkpoint, const bool missing_participants) {
    assert(participants.size() <= std::numeric_limits<byte>::max());
    running_ = true;

    for (auto& [id, participant] : participants) {
        logger_.trace("Déplacement du socket du participant {}.", id);

        Player player { id, participant.name, game().player(), game().itemsList(), game().bonuses };

        players_.insert({ id, std::move(player) });
        connections_.insert({ id, std::move(participant.socket) });
    }

    stats_ = { game().global() };

    SessionDataFactory start_msg;
    start_msg.makeData(DataType::Start);

    sendToAll(start_msg.dataWithLength());

    Gameplay interface { *this };
    word beginning;
    if (checkpoint.empty()) {
        logger_.info("Nouvelle partie sur \"{}\".", game().name);
        playScene(interface, INTRO);

        if (!game().globalStats.empty())
            interface.print("Statistiques globales :");

        for (const auto& [name, stat] : game().globalStats) {
            const auto [init, limits, capped, hidden, main] { stat };
            const int value { init() };

            assert((static_cast<int>(init.dices) + init.bonus) >= limits.min);

            stats_.setLimits(name, limits.min, capped ? std::min(value, limits.max) : limits.max);
            stats_.set(name, value);
            stats_.setHidden(name, hidden);
            stats_.setMain(name, main);

            const std::string stat_msg { initStatMsg(init, name, value) };
            if (!hidden) {
                SessionDataFactory global_stat;
                interface.sendGlobalStat(name);

                sendToAll(global_stat.dataWithLength());
                interface.print(stat_msg);
            }

            logger_.info("[Global stats] {}", stat_msg);
        }

        initPlayers(interface);

        if (game().voteLeader)
            interface.voteForLeader();
        else
            switchLeader(players_.cbegin()->first);

        beginning = 1;
    } else {
        GameState state;
        try {
            state = gameBuilder().load(checkpoint);
        } catch (const std::exception& err) {
            end(participants);
            throw CheckpointLoadingError { err.what() };
        }

        for (const auto& [name, stat] : state.global) {
            const auto [value, limits, hidden, main] { stat };
            const auto [min, max] { limits };
            assert(!(value < min || value > max));

            stats().setLimits(name, min, max);
            stats().set(name, value);
            stats().setHidden(name, hidden);
            stats().setMain(name, main);

            SessionDataFactory global_stat;
            interface.sendGlobalStat(name);

            sendToAll(global_stat.dataWithLength());
        }

        ParticipantsValidity participants_validity { ParticipantsValidity::Ok };
        for (const auto& player : players_) {
            if (state.players.count(player.first) == 0) {
                participants_validity = ParticipantsValidity::UnknownPlayer;
                break;
            }
        }

        if (participants_validity == ParticipantsValidity::Ok) {
            for (const auto& [id, p_state] : state.players) {
                if (players_.count(id) == 1) {
                    restaurePlayer(id, p_state);
                } else if (!missing_participants) {
                    participants_validity = ParticipantsValidity::LessMembers;
                    break;
                }
            }
        }

        if (participants_validity != ParticipantsValidity::Ok) {
            end(participants);

            std::vector<byte> expected_players;
            expected_players.resize(state.players.size(), 0);

            std::transform(state.players.cbegin(), state.players.cend(), expected_players.begin(), [](const auto& ps) -> byte {
                return ps.first;
            });

            throw InvalidIDs { expected_players, participants_validity };
        }

        beginning = state.scene;

        if (players_.count(state.leader) == 0) {
            if (game().voteOnLeaderDeath)
                interface.voteForLeader();
            else
                switchLeader(players_.cbegin()->first);
        } else {
            switchLeader(state.leader);
        }
    }

    logger_.debug("global={}", stats().values());
    for (const auto& [id, player] : players_) {
        logger_.debug("player:{}={}", id, player);
        interface.sendInfos(id);
    }
    first_state_ = false;

    for (Next next { beginning }; next && running(); next = playScene(interface, *next));

    end(participants);
}

Next Session::playScene(Gameplay& interface, const word id) {
    logger_.info("Passage à la scène {}.", id);
    current_scene_ = id;
    const Scene scene { gameBuilder().buildScene(id) };

    SessionDataFactory switch_msg;
    switch_msg.makeSwitch(id);

    sendToAll(switch_msg.dataWithLength());

    for (const Instruction& step : scene) {
        if (!running())
            break;

        try {
            const Next result { step(interface) };

            if (result)
                return result;
        } catch (const CancelledRequest&) {
            break;
        }
    }

    if (id != INTRO)
        logger_.info("Fin du jeu.");

    return {};
}

void Session::initPlayers(Gameplay& interface) {
    for (auto& target : players_)
        initPlayer(interface, target.second);
}

void Session::initPlayer(Gameplay& interface, Player& target) {
    interface.print("Stats de " + target.name());
    for (const auto& [name, stat] : game().playerStats) {
        const auto [init, limits, capped, hidden, main] { stat };
        const int value { init() };

        target.stats().setLimits(name, limits.min, capped ? std::min(value, limits.max) : limits.max);
        target.stats().set(name, value);
        target.stats().setHidden(name, hidden);
        target.stats().setMain(name, main);

        const std::string stat_msg { initStatMsg(init, name, value) };
        logger_.info("[Player {} stats] {}", target.id(), stat_msg);
        interface.print(stat_msg);
    }

    interface.print("Inventaires de " + target.name());
    for (const auto& [name, inv] : game().playerInventories) {
        const auto& [limit, items, initial] { inv };

        std::string size_msg { name + " - taille : " };
        InventorySize size;
        if (limit) {
            const DiceFormula& formula { *limit };
            size = formula();

            size_msg += formula.dices == 0
                    ? std::to_string(*size)
                    : std::to_string(formula.dices) + " dé(s) 6 + " + std::to_string(formula.bonus) + " = " + std::to_string(*size);
        } else {
            size = {};
            size_msg += "infinie";
        }

        interface.print(size_msg);

        target.inventory(name).setMaxSize(size);
        if (initial.empty())
            continue;

#ifndef NDEBUG
        const InventorySize capacity { target.inventory(name).maxSize() };
#endif
        assert(!capacity || std::accumulate(initial.cbegin(), initial.cend(), 0, [](const std::size_t s, const auto& i) { return s + i.second; }) <= capacity);

        std::string initial_msg { name + " - contenu initial :" };
        for (const auto& [item, qty] : initial) {
            target.add(name, item, qty);
            initial_msg += ' ' + item + '*' + std::to_string(qty);
        }

        logger_.info("[Player {} inventories] {}", target.id(), initial_msg);
        interface.print(initial_msg);
    }
}

void Session::restaurePlayer(const byte id, const PlayerState& state) {
    Player& target { player(id) };

    for (const auto& [name, stat] : state.stats) {
        const auto [value, limits, hidden, main] { stat };
        const auto [min, max] { limits };
        assert(!(stat.value < min || stat.value > max));

        target.stats().setLimits(name, min, max);
        target.stats().set(name, stat.value);
        target.stats().setHidden(name, hidden);
        target.stats().setMain(name, main);
    }

    for (const auto& [inv, content] : state.inventories) {
#ifndef NDEBUG
        assert(target.inventory(inv).setMaxSize(state.inventoriesMaxCapacity.at(inv)));
#else
        target.inventory(inv).setMaxSize(state.inventoriesMaxCapacity.at(inv));
#endif

        for (const auto& [item, qty] : content)
            target.add(inv, item, qty);
    }
}

void Session::reset() {
    stats_ = {};
    players_.clear();
    connections_.clear();
    leader_ = 0;
    last_states_.clear();
    current_scene_ = 0;
    running_ = false;
    first_state_ = true;
}

void Session::switchLeader(const byte id) {
    assert(players().count(id) == 1);
    leader_ = id;

    SessionDataFactory switch_data;
    switch_data.makeLeaderSwitch(id);

    sendToAll(switch_data.dataWithLength());
}

std::string Session::checkpoint(const std::string& chkpt_name, const word id) const {
    if (current_scene_ == INTRO)
        throw IntroductionCheckpoint {};

    PlayersStates states;
    std::transform(players_.cbegin(), players_.cend(), std::inserter(states, states.begin()), [](const auto& state) -> PlayersStates::value_type {
        const auto& [id, player] { state };

        const Stats& stats { player.stats().raw() };
        std::unordered_map<std::string, InventoryContent> inventories;
        std::unordered_map<std::string, InventorySize> inventoriesMaxCapacity;
        for (const auto& [name, inventory] : player.inventories()) {
            inventories.insert({ name, inventory.content() });
            inventoriesMaxCapacity.insert({ name, inventory.maxSize() });
        }

        return { id, PlayerState { stats, inventories, inventoriesMaxCapacity } };
    });

    return gameBuilder().save(chkpt_name, { id, stats().raw(), leader(), states });
}

Players Session::players() {
    std::map<byte, Player*> ptrs;
    std::transform(players_.begin(), players_.end(), std::inserter(ptrs, ptrs.end()), [](auto& p) -> std::pair<byte, Player*> {
        return { p.first, &p.second };
    });

    return ptrs;
}

template<typename T> std::map<byte, const T*> constPtrMap(const std::map<byte, T>& map) {
    std::map<byte, const T*> const_ptrs;

    std::transform(map.cbegin(), map.cend(), std::inserter(const_ptrs, const_ptrs.end()), [](const auto& v) -> std::pair<byte, const T*> {
        return { v.first, &v.second };
    });

    return const_ptrs;
}

ConstPlayers Session::players() const {
    return constPtrMap(players_);
}

Player& Session::player(const byte id) {
    if (players_.count(id) == 0)
        throw UnknownPlayer { id };

    return players_.at(id);
}

const Player& Session::player(const byte id) const {
    if (players_.count(id) == 0)
        throw UnknownPlayer { id };

    return players_.at(id);
}

PlayerStateChanges Session::getChanges(const byte id) {
    Player& p { player(id) };
    PlayerState& last { last_states_[id] };
    auto& [stats, inventories, capacities] { last };

    PlayerStateChanges changes;
    for (const auto& [name, stat] : p.stats()) {
        if (first_state_ || (!p.stats().hidden(name) && stat != stats.at(name))) {
            changes.statsChanges[name] = stat;
            stats[name] = stat;
        }
    }

    for (const auto& [name, inv] : p.inventories()) {
        for (const auto& [item, qty] : inv.content()) {
            if (first_state_ || qty != inventories.at(name).at(item)) {
                changes.itemsChanges[name][item] = qty;
                inventories[name][item] = qty;
            }
        }

        const InventorySize inv_max_capacity { inv.maxSize() };
        if (first_state_ || inv_max_capacity != capacities.at(name)) {
            changes.capacitiesChanges[name] = inv_max_capacity;
            capacities[name] = inv_max_capacity;
        }
    }

    return changes;
}

Replies Session::request(const byte target, const Data& data, ReplyController controller, const bool wait) {
    RequestCtx ctx;

    if (target == ALL_PLAYERS) {
        for (auto& [id, connection] : connections_)
            ctx.players.insert({ id, &connection });
    } else {
        ctx.players.insert({ target, &connection(target) });
    }

    ctx.counter = 0;
    ctx.limit = wait ? ctx.players.size() : 1;
    const ulong to_handle { ctx.players.size() };

    std::map<byte, ReplyHandler> handlers;
    std::transform(ctx.players.cbegin(), ctx.players.cend(), std::inserter(handlers, handlers.end()), [&ctx, &controller, this](const auto p) -> std::pair<byte, ReplyHandler> {
        return { p.first, ReplyHandler { executor_, logger_, ctx, controller, p.first } };
    });

    const io::const_buffer buffer { trunc(data) };
    for (auto [id, player] : ctx.players) {
        player->async_send(buffer, io::bind_executor(executor_, std::bind(&ReplyHandler::handle, &handlers.at(id), std::placeholders::_1, std::placeholders::_2)));
    }

    logger_.trace("En attente de {} réponses...", ctx.limit);
    while (ctx.counter < to_handle && running())
        std::this_thread::sleep_for(std::chrono::milliseconds { 1 });
    logger_.trace("Réponses reçues.");

    SessionDataFactory end;
    end.makeRequest(Request::End);
    const io::const_buffer ending_buffer { trunc(end.dataWithLength()) };

    for (const auto [id, player] : ctx.players) {
        const auto b { ctx.errorIDs.cbegin() };
        const auto e { ctx.errorIDs.cend() };

        if (std::find(b, e, id) == e) {
            const ErrCode end_err { trySend(*player, ending_buffer) };

            if (end_err) {
                logPlayerError(logger_, id, end_err.message());
                ctx.errorIDs.push_back(id);
            }
        }
    }

    // Méthode de déconnexion différente pour éviter d'essayer d'envoyer des paquets
    // d'informations aux joueurs ayant crash
    for (const byte player : ctx.errorIDs)
        removePlayer(player);

    if (!playersRemaining())
        throw NoPlayerRemaining {};

    if (players_.count(leader()) == 0)
        switchLeader(players_.cbegin()->first);

    for (const byte player : ctx.errorIDs) {
        SessionDataFactory crash_data;
        crash_data.makeCrash(player);

        sendToAll(crash_data.dataWithLength());
    }

    logger_.debug("replies={}", ctx.replies);
    logger_.debug("errorIDs={}", ctx.errorIDs);

    if (!running())
        throw CancelledRequest {};

    return ctx.replies;
}

void Session::sendTo(const byte target_id, const Data& data) {
    if (target_id == ALL_PLAYERS) {
        sendToAll(data);
        return;
    }

    const ErrCode err { trySend(connection(target_id), trunc(data)) };

    if (err) {
        logPlayerError(logger_, target_id, err.message());
        disconnect(target_id, true);
    }
}

void Session::sendToAll(const Data& data) {
    const io::const_buffer buffer { trunc(data) };

    for (auto& [id, connection] : connections_) {
        const ErrCode err { trySend(connection, buffer) };

        if (err) {
            logPlayerError(logger_, id, err.message());
            disconnect(id, true);
        }
    }
}

} // namespace Rbo