#include <Rbo/Session.hpp>

#include <spdlog/logger.h>
#include <spdlog/fmt/ostr.h>
#include <Rbo/SessionDataFactory.hpp>
#include <Rbo/GameBuilder.hpp>
#include <Rbo/Gameplay.hpp>
#include <Rbo/ReplyHandler.hpp>

namespace Rbo {

InvalidIDs::InvalidIDs(const std::vector<byte>& expected_ids, const EntrantsValidity type) : std::logic_error { "Invalid IDs" }, expectedIDs { expected_ids }, errType { type } {
    assert(type != EntrantsValidity::Ok);

    msg = "Expected IDs :";
    for (const byte expected : expectedIDs)
        msg += ' ' + std::to_string(expected) + ';';
}

const char* CanceledRequest::what() const noexcept {
    return "CanceledRequest";
}

namespace  {

std::string initStatMsg(const DicesRoll& init, const std::string& name, const int value) {
    std::string msg { name + " = " };
    if (init.dices == 0)
        msg += std::to_string(init.bonus);
    else
        msg += std::to_string(init.dices) + " dice(s) 6 + " + std::to_string(init.bonus) + " = " + std::to_string(value);

    return msg;
}

}

tcp::socket& Session::connection(const byte id) {
    assert(connections_.count(id) == 1);

    return connections_.at(id);
}

void Session::logPlayerError(const byte player, const std::string& err) {
    logger_.error("Player {} : {}", player, err);
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
      running_ { false },
      game_builer_ { g_builder } {}

void Session::begin(Entrants& entrants) {
    for (auto& [id, participant] : entrants) {
        logger_.trace("Moving socket of entrant [{}]...", id);

        Player player { id, participant.name, game().player(), game().itemsList(), game().bonuses };

        players_.insert({ id, std::move(player) });
        connections_.insert({ id, std::move(participant.socket) });
    }

    SessionDataFactory start_msg;
    start_msg.makeStart(game().name);

    sendToAll(start_msg.dataWithLength());
}

void Session::end(Entrants& entrants) {
    SessionDataFactory stop_msg;
    stop_msg.makeData(DataType::Stop);

    sendToAll(stop_msg.dataWithLength());

    std::vector<byte> error_ids;
    for (auto& [id, participant] : entrants) {
        if (connections_.count(id) == 1) {
            logger_.trace("Moving socket of entrant [{}]...", id);
            participant.socket = std::move(connection(id));
        } else {
            error_ids.push_back(id);
        }
    }

    for (const byte id : error_ids) {
        logger_.trace("Removing entrant [{}]...", id);
        entrants.erase(id);
    }

    stop();
};

word Session::newGame() {
    logger_.info("New game on \"{}\".", game().name);

    for (const auto& [name, stat] : game().globalStats) {
        const auto [init, limits, capped, hidden, main] { stat };

        RollResult result { init() };
        const int value { result.total() };

        rolls_results_.globalStats.insert({ name, std::move(result) });

        stats_.setLimits(name, limits.min, capped ? std::min(value, limits.max) : limits.max);
        stats_.set(name, value);
        stats_.setHidden(name, hidden);
        stats_.setMain(name, main);
    }

    for (auto& [id, player] : players_) {
        // On initialise les entrées dans la collection pour les résultats des lancés de dés du joueur
        rolls_results_.playersStats.insert({ id, {} });
        rolls_results_.playersInvsCapacity.insert({ id, {} });

        initPlayer(player);
    }

    switchLeader(players_.cbegin()->first);

    return INTRO;
}

word Session::gameFromCheckpoint(const std::string& final_name, const bool missing_entrants) {
    GameState checkpoint;
    try {
        checkpoint = gameBuilder().load(final_name);
    } catch (const std::exception& err) {
        throw CheckpointLoadingError { err.what() };
    }

    for (const auto& [name, stat] : checkpoint.global) {
        const auto [value, limits, hidden, main] { stat };
        const auto [min, max] { limits };
        assert(!(value < min || value > max));

        stats().setLimits(name, min, max);
        stats().set(name, value);
        stats().setHidden(name, hidden);
        stats().setMain(name, main);

        SessionDataFactory global_stat;
        global_stat.makeGlobalStat(name, stat);

        sendToAll(global_stat.dataWithLength());
    }

    const EntrantsValidity entrants_validity { checkEntrants(checkpoint, missing_entrants) };
    if (entrants_validity != EntrantsValidity::Ok) {
        std::vector<byte> expected_players;
        expected_players.resize(checkpoint.players.size(), 0);

        std::transform(checkpoint.players.cbegin(), checkpoint.players.cend(), expected_players.begin(), [](const auto& ps) -> byte {
            return ps.first;
        });

        throw InvalidIDs { std::move(expected_players), entrants_validity };
    }

    for (const auto& [player_id, state] : checkpoint.players) {
        if (players_.count(player_id) == 1)
            restorePlayer(player_id, state);
#ifndef NDEBUG // Pour éviter d'avoir un bloc else vide
        else
            assert(missing_entrants);
#endif
    }

    if (players_.count(checkpoint.leader))
        switchLeader(checkpoint.leader);

    return checkpoint.scene;
}

EntrantsValidity Session::checkEntrants(const GameState& checkpoint, const bool missing_entrants) const {
    EntrantsValidity entrants_validity { EntrantsValidity::Ok };
    for (const auto& player : players_) {
        if (checkpoint.players.count(player.first) == 0) {
            entrants_validity = EntrantsValidity::UnknownPlayer;
            break;
        }
    }

    if (entrants_validity == EntrantsValidity::Ok) {
        for (const auto& [id, p_state] : checkpoint.players) {
            if (players_.count(id) == 0 && !missing_entrants) {
                entrants_validity = EntrantsValidity::LessMembers;
                break;
            }
        }
    }

    return entrants_validity;
}

void Session::initPlayer(Player& target) {
    std::unordered_map<std::string, RollResult>& stats_rolls_result { rolls_results_.playersStats.at(target.id()) };
    std::unordered_map<std::string, RollResult>& invs_capacity_rolls_result { rolls_results_.playersInvsCapacity.at(target.id()) };

    for (const auto& [name, stat] : game().playerStats) {
        const auto [init, limits, capped, hidden, main] { stat };
        const RollResult result { init() };
        const int value { result.total() };

        stats_rolls_result.insert({ name, result });

        target.stats().setLimits(name, limits.min, capped ? std::min(value, limits.max) : limits.max);
        target.stats().set(name, value);
        target.stats().setHidden(name, hidden);
        target.stats().setMain(name, main);

        const std::string stat_msg { initStatMsg(init, name, value) };
        logger_.info("[Player {} stats] {}", target.id(), stat_msg);
    }

    for (const auto& [name, inv] : game().playerInventories) {
        const auto& [limit, items, initial] { inv };

        std::string size_msg { name + " - size : " };
        InventorySize size;
        if (limit) {
            const DicesRoll& dice_roll { *limit };

            RollResult result { dice_roll() };

            size = result.total();

            invs_capacity_rolls_result.insert({ name, std::move(result) });

            size_msg += dice_roll.dices == 0 ? std::to_string(*size)
                                             : std::to_string(dice_roll.dices) + " dices(s) 6 + " + std::to_string(dice_roll.bonus) + " = " + std::to_string(*size);
        } else {
            size = {};
            size_msg += "Inf";
        }

        target.inventory(name).setCapacity(size);
        if (initial.empty())
            continue;

#ifndef NDEBUG
        const InventorySize capacity { target.inventory(name).capacity() };
#endif
        assert(!capacity || std::accumulate(initial.cbegin(), initial.cend(), std::size_t { 0 }, [](const std::size_t s, const auto& i) { return s + i.second; }) <= capacity);

        std::string initial_msg { name + " - initial content :" };
        for (const auto& [item, qty] : initial) {
            target.add(name, item, qty);
            initial_msg += ' ' + item + '*' + std::to_string(qty);
        }

        logger_.info("[Player {} inventories] {}", target.id(), initial_msg);
    }
}

void Session::restorePlayer(const byte id, const PlayerState& state) {
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
        assert(target.inventory(inv).setCapacity(state.capacities.at(inv)));
#else
        target.inventory(inv).setCapacity(state.capacities.at(inv));
#endif

        for (const auto& [item, qty] : content)
            target.add(inv, item, qty);
    }
}

void Session::globalDiceRolls(Gameplay& interface) const {
    const Message& dice_roll_msg { game().messages.at("stat_dice_roll") };
    const std::string& msg { dice_roll_msg ? *dice_roll_msg : "Dice roll for stat \"{stat}\"" };

    for (const auto& [name, stat_descriptor] : game().globalStats) {
        if (stat_descriptor.initialValue.dices == 0)
            continue;

        const DiceRollResults stat_value { { leader(), rolls_results_.globalStats.at(name) } };
        interface.askDiceRoll(leader(), fmt::format(msg, fmt::arg("stat", name)), stat_descriptor.initialValue, stat_value);
    }
}

void Session::playersDiceRolls(Gameplay& interface) const {
    const Message& stat_dice_roll_msg { game().messages.at("stat_dice_roll") };
    const std::string& stat_msg { stat_dice_roll_msg ? *stat_dice_roll_msg : "Dice roll for stat \"{stat}\"" };

    for (const auto& [name, stat_descriptor] : game().playerStats) {
        if (stat_descriptor.initialValue.dices == 0)
            continue;

        DiceRollResults stats_results;
        for (auto& [id, player_stats] : rolls_results_.playersStats)
            stats_results.insert({ id, player_stats.at(name) });

        interface.askDiceRoll(ALL_PLAYERS, fmt::format(stat_msg, fmt::arg("stat", name)), stat_descriptor.initialValue, stats_results);
    }

    const Message& capacity_dice_roll_msg { game().messages.at("inventory_capacity_dice_roll") };
    const std::string& capacity_msg { capacity_dice_roll_msg ? *capacity_dice_roll_msg : "Dice roll for capacity of inventory \"{inventory}\"" };

    for (const auto& [name, inv_descriptor] : game().playerInventories) {
        if (!inv_descriptor.limit || inv_descriptor.limit->dices == 0)
            continue;

        DiceRollResults inv_capacity_results;
        for (const auto& [id, player_invs_capacity] : rolls_results_.playersInvsCapacity)
            inv_capacity_results.insert({ id, player_invs_capacity.at(name) });

        interface.askDiceRoll(ALL_PLAYERS, fmt::format(capacity_msg, fmt::arg("inventory", name)), *inv_descriptor.limit, inv_capacity_results);
    }
}

void Session::start(Entrants& initial_entrants_data, const std::string& checkpoint, const bool missing_entrants) {
    running_ = true;
    logger_.info("Session started.");

    begin(initial_entrants_data);

    try {
        stats_ = { game().global() };

        const bool new_game { checkpoint.empty() };
        const word beginning { checkpoint.empty() ? newGame() : gameFromCheckpoint(checkpoint, missing_entrants) };

        Gameplay interface { *this };

        if (new_game) {
            globalDiceRolls(interface);
            playersDiceRolls(interface);
        }

        for (const auto& player : players_) {
            const byte id { player.first };

            interface.initCache(id);
        }

        if (new_game && game().voteLeader)
            interface.voteForLeader();

        if (!leader_) {
            if (game().voteLeader)
                interface.voteForLeader();
            else
                switchLeader(players_.cbegin()->first);
        }

        logger_.debug("Global : {}", StatsValueWrapper { stats().values() });
        for (const auto& [id, player] : players_) {
            logger_.debug("Player {} : {}", id, player);
            interface.initCache(id);

            interface.sendPlayerUpdate(id);
        }

        for (const auto& stat : stats())
            interface.sendGlobalStat(stat.first);

        for (Next next { beginning }; next && running(); next = playScene(interface, *next));
    } catch (const std::exception& err) {
        end(initial_entrants_data);
        throw;
    }

    end(initial_entrants_data);
}

Next Session::playScene(Gameplay& interface, const word id) {
    logger_.info("Go to scene {}.", id);
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
        } catch (const CanceledRequest&) {
            break;
        }
    }

    if (id != INTRO)
        logger_.info("Game end.");

    return {};
}

void Session::reset() {
    stats_ = {};
    rolls_results_ = {};
    players_.clear();
    connections_.clear();
    leader_ = 0;
    current_scene_ = 0;
    running_ = false;
}

byte Session::leader() const {
    if (!leader_)
        throw UninitializedLeader {};

    return *leader_;
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

    PlayersState states;
    std::transform(players_.cbegin(), players_.cend(), std::inserter(states, states.begin()), [](const auto& state) -> PlayersState::value_type {
        const auto& [id, player] { state };

        const Stats& stats { player.stats().raw() };
        std::unordered_map<std::string, InventoryContent> inventories;
        std::unordered_map<std::string, InventorySize> capacities;
        for (const auto& [name, inventory] : player.inventories()) {
            inventories.insert({ name, inventory.content() });
            capacities.insert({ name, inventory.capacity() });
        }

        return { id, PlayerState { stats, inventories, capacities } };
    });

    return gameBuilder().save(chkpt_name, { id, stats().raw(), leader(), states });
}

std::vector<byte> Session::ids() const {
    std::vector<byte> ids;
    ids.resize(count(), 0);

    std::transform(players_.cbegin(), players_.cend(), ids.begin(), [](const auto& p) -> byte {
        return p.first;
    });

    return ids;
}

Players Session::players() {
    std::map<byte, Player*> ptrs;
    std::transform(players_.begin(), players_.end(), std::inserter(ptrs, ptrs.end()), [](auto& p) -> std::pair<byte, Player*> {
        return { p.first, &p.second };
    });

    return ptrs;
}

ConstPlayers Session::players() const {
    std::map<byte, const Player*> const_players;

    std::transform(players_.cbegin(), players_.cend(), std::inserter(const_players, const_players.end()), [](const auto& p) -> std::pair<byte, const Player*> {
        return { p.first, &p.second };
    });

    return const_players;
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

Replies Session::request(const byte targets_id, const Data& data, ReplyController controller, const bool first_reply_only, const bool wait_all_replies) {
    const RequestCtxPtr ctx { std::make_shared<RequestCtx>() };
    const bool all_players { targets_id == ALL_PLAYERS };

    byte targets_count { 0 };
    for (auto& [id, connection] : connections_) {
        const bool targetted { all_players || id == targets_id };

        ctx->players.insert({ id, RequestProfile { &connection, targetted } });
        if (targetted)
            targets_count++;
    }

    ulong replies_to_receive;
    if (first_reply_only) {
        ctx->repliesToAccept = 1;
        replies_to_receive = wait_all_replies ? targets_count : 1;
    } else {
        ctx->repliesToAccept = targets_count;
        replies_to_receive = targets_count;
    }

    const io::const_buffer buffer { trunc(data) };
    std::map<byte, ReplyHandler> handlers;
    for (auto [id, player] : ctx->players) {
        if (player.targetted) {
            handlers.insert({ id, ReplyHandler { executor_, logger_, ctx, controller, id } });

            const byte player_id { id }; // Nécessaire pour pouvoir être capturé par la lambda
            player.connection->async_send(buffer, io::bind_executor(executor_, [&handlers, player_id](const ErrCode err, const std::size_t len) {
                handlers.at(player_id).handle(err, len);
            }));
        } else {
            const ErrCode send_err { trySend(*player.connection, buffer) };

            if (send_err) {
                ctx->players.erase(ctx->players.find(id));
                ctx->errorIDs.push_back(id);
            }
        }
    }

    logger_.info("Waiting for {} replies in total...", replies_to_receive);
    while (ctx->repliesHandled < replies_to_receive && running())
        std::this_thread::sleep_for(std::chrono::milliseconds { 1 });
    logger_.info("{} replies received.", ctx->repliesHandled.load());

    ctx->requestDone = true;
    for (auto& player : ctx->players)
        player.second.connection->cancel();

    SessionDataFactory end;
    end.makeData(DataType::FinishRequest);
    const io::const_buffer ending_buffer { trunc(end.dataWithLength()) };

    for (const auto [id, player] : ctx->players) {
        const auto b { ctx->errorIDs.cbegin() };
        const auto e { ctx->errorIDs.cend() };

        if (std::find(b, e, id) == e) {
            const ErrCode end_err { trySend(*player.connection, ending_buffer) };

            if (end_err) {
                logPlayerError(id, end_err.message());
                ctx->errorIDs.push_back(id);
            }
        }
    }

    // Méthode de déconnexion différente pour éviter d'essayer d'envoyer des paquets
    // d'informations aux joueurs ayant crash
    for (const byte player : ctx->errorIDs)
        removePlayer(player);

    if (!playersRemaining())
        throw NoPlayerRemaining {};

    if (players_.count(leader()) == 0)
        switchLeader(players_.cbegin()->first);

    for (const byte player : ctx->errorIDs) {
        SessionDataFactory crash_data;
        crash_data.makeCrash(player);

        sendToAll(crash_data.dataWithLength());
    }

    logger_.info("Replies : {}", RepliesWrapper { ctx->replies });
    if (!ctx->errorIDs.empty())
        logger_.warn("Crashed players : {}", ByteVecWrapper { ctx->errorIDs });

    if (!running())
        throw CanceledRequest {};

    return ctx->replies;
}

void Session::sendTo(const byte target_id, const Data& data) {
    if (target_id == ALL_PLAYERS) {
        sendToAll(data);
        return;
    }

    const ErrCode err { trySend(connection(target_id), trunc(data)) };

    if (err) {
        logPlayerError(target_id, err.message());
        disconnect(target_id, true);
    }
}

void Session::sendToAll(const Data& data) {
    const io::const_buffer buffer { trunc(data) };

    for (const byte id : ids()) {
        const ErrCode err { trySend(connection(id), buffer) };

        if (err) {
            logPlayerError(id, err.message());
            disconnect(id, true);
        }
    }
}

} // namespace Rbo
