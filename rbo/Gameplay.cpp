#include <Rbo/Gameplay.hpp>

#include <Rbo/Session.hpp>
#include <Rbo/SessionDataFactory.hpp>

namespace Rbo {

namespace {

void confirmController(const byte reply) {
    if (reply != 0)
        throw InvalidReply { ReplyValidity::NotConfirmError };
}

struct RangeController {
    byte min;
    byte max;

    void operator()(const byte reply) const {
        if (reply < min || reply > max)
            throw InvalidReply { ReplyValidity::OutOfRangeError };
    }
};

} // namespace Controllers

StatsManager& Gameplay::global() {
    return ctx_.stats();
}

const Game& Gameplay::game() const {
    return ctx_.game();
}

const RestProperties& Gameplay::rest() const {
    return game().rest;
}

std::string Gameplay::checkpoint(const std::string& chkpt, const word id) const {
    return ctx_.checkpoint(chkpt, id);
}

Player& Gameplay::player(const byte id) {
    return ctx_.player(id);
}

std::vector<byte> Gameplay::players() const {
    return ctx_.ids();
}

std::vector<byte> Gameplay::activePlayers() const {
    return ctx_.aliveIDs();
}

OptionsList Gameplay::names() const {
    const Players players { ctx_.players() };

    OptionsList names;
    names.resize(players.size());

    std::transform(players.cbegin(), players.cend(), names.begin(), [](const auto p) -> std::string {
        return p.second->name();
    });

    return names;
}

std::size_t Gameplay::count() const {
    return ctx_.count();
}

byte Gameplay::leader() const {
    return ctx_.leader();
}

void Gameplay::switchLeader(const byte id) {
    ctx_.switchLeader(id);
}

std::optional<byte> Gameplay::votePlayer(const std::string& msg, const byte target) {
    const OptionsList players_name { names() };
    const std::optional<byte> player_number { vote(ask(target, msg, players_name)) };

    if (!player_number)
        return {};

    const std::string& selected_name { players_name.at(*player_number) };
    const std::vector<byte> players_id { players() };
    const auto selected_player = std::find_if(players_id.cbegin(), players_id.cend(), [this, &selected_name](const byte p_id) {
        return player(p_id).name() == selected_name;
    });

    assert(selected_player != players_id.cend());
    return *selected_player;
}

void Gameplay::voteForLeader() {
    const std::optional<byte> new_leader { vote(ask(ACTIVE_PLAYERS, "Qui doit devenir leader ?", names(), true)) };

    // S'il n'y a pas de nouveau leader, alors c'est qu'il ne reste plus aucun joueur vivant, donc inutile et impossible de changer de leader.
    if (new_leader)
        switchLeader(*new_leader);
}

Replies Gameplay::ask(const byte target, const std::string& msg, const OptionsList& options, const bool first_reply_only, const bool wait_all_replies) {
    SessionDataFactory data_factory;
    data_factory.makeOptions(target, msg, options);

    return ctx_.request(target, data_factory.dataWithLength(), RangeController { 1, static_cast<byte>(options.size()) }, first_reply_only, wait_all_replies);
}

Replies Gameplay::askNumber(const byte target, const std::string& msg, const byte min, const byte max, const bool first_reply_only, const bool wait_all_replies) {
    SessionDataFactory data_factory;
    data_factory.makeRange(target, msg, min, max);

    return ctx_.request(target, data_factory.dataWithLength(), RangeController { min, max }, first_reply_only, wait_all_replies);
}

Replies Gameplay::askConfirm(const byte target, const bool first_reply_only) {
    SessionDataFactory data_factory;
    data_factory.makeRequest(Request::Confirm, target);

    // Il n'y a aucun intérêt à ne pas prendre en compte une confirmation reçue car elle serait arrivée trop tard
    return ctx_.request(target, data_factory.dataWithLength(), confirmController, first_reply_only, false);
}

Replies Gameplay::askYesNo(const byte target, const std::string& question, const bool first_reply_only, const bool wait_all_replies) {
    SessionDataFactory data_factory;
    data_factory.makeYesNoQuestion(target, question);

    return ctx_.request(target, data_factory.dataWithLength(), RangeController { 0, 1 }, first_reply_only, wait_all_replies);
}

Replies Gameplay::askDiceRoll(const byte target, const std::string& msg, const DicesRoll& formula, const DiceRollResults& results) {
    if (target != ALL_PLAYERS && target != ACTIVE_PLAYERS && results.count(target) == 0)
        throw InvalidDiceRollResults { target };

    SessionDataFactory data_factory;
    data_factory.makeDiceRoll(target, msg, formula.dices, formula.bonus, results);

    return ctx_.request(target, data_factory.dataWithLength(), confirmController, false, true);
}

PlayerCheckingResult Gameplay::checkPlayer(const byte id) {
    Player& p { player(id) } ;

    const std::vector<DeathCondition>& conditions { game().deathConditions };
    const auto death = std::find_if(conditions.cbegin(), conditions.cend(), [&p](const auto& dc) {
        return dc.dieIf.test(p.stats());
    });

    const bool alive { death == conditions.cend() };

    if (alive)
        return { false, false, false };

    p.kill(death->deathMessage);
    sendPlayerUpdate(id);

    if (!ctx_.anyPlayerAlive()) {
        const Message& game_over { game().messages.at("all_players_dead") };

        printImportant(game_over ? *game_over : "All players are dead !");
        askConfirm(ALL_PLAYERS);
        ctx_.stop();

        return { true, false, true };
    }

    const bool leader_switch { leader() == id };
    if (leader_switch)
        switchLeader(*activePlayers().cbegin());

    return { true, leader_switch, false };
}

bool Gameplay::checkGame() {
    const std::vector<EndCondition>& conditions { game().gameEndConditions };
    const auto stop = std::find_if(conditions.cbegin(), conditions.cend(), [this](const auto& ec) {
        return ec.stopIf.test(global());
    });

    const bool continue_game { stop == conditions.cend() };

    if (!continue_game) {
        ctx_.stop();
        printImportant(stop->endMessage);
    }

    return continue_game;
}

void Gameplay::print(const std::string& txt, const byte target) {
    SessionDataFactory data_factory;
    data_factory.makeNormalText(txt);

    ctx_.sendTo(target, data_factory.dataWithLength());
}

void Gameplay::printImportant(const std::string& txt, const byte target) {
    SessionDataFactory data_factory;
    data_factory.makeImportantText(txt);

    ctx_.sendTo(target, data_factory.dataWithLength());
}

void Gameplay::printNote(const std::string& note, const byte target) {
    SessionDataFactory data_factory;
    data_factory.makeNote(note);

    ctx_.sendTo(target, data_factory.dataWithLength());
}

void Gameplay::printTitle(const std::string& title) {
    SessionDataFactory data_factory;
    data_factory.makeTitle(title);

    ctx_.sendTo(ALL_PLAYERS, data_factory.dataWithLength());
}

void Gameplay::sendGlobalStat(const std::string& stat) {
    SessionDataFactory data_factory;
    data_factory.makeGlobalStat(stat, global().raw().at(stat));

    ctx_.sendToAll(data_factory.dataWithLength());
}

void Gameplay::initCache(const byte player_id) {
    Player& target { player(player_id) };

    InventoriesContent inventories;
    InventoriesSize capacities;

    for (const auto& [name, inv] : target.inventories()) {
        inventories.insert({ name, inv.content() });
        capacities.insert({ name, inv.capacity() });
    }

    PlayerCache updated_cache { false,  PlayerState { Death {}, target.stats().raw(), std::move(inventories), std::move(capacities) } };
    players_cache_.insert({ player_id, std::move(updated_cache) });
}

void Gameplay::sendPlayerUpdate(const byte id) {
    Player& p_updated { player(id) };
    PlayerCache& cache { players_cache_.at(id) };
    PlayerState& cached_state { cache.playerState };
    bool& cache_initialized { cache.initialized };

    PlayerUpdate update;
    update.death = p_updated.alive() ? Death {} : Death { p_updated.death() };

    for (const auto& [name, stat] : p_updated.stats()) {
        Stat& cached_stat { cached_state.stats.at(name) };

        if (!cache_initialized || stat != cached_stat) {
            update.stats.insert({ name, stat });

            if (stat.hidden) {
                cached_stat.hidden = true;
                cached_stat.main = stat.main;
            } else {
                cached_stat = stat;
            }
        }
    }

    for (const auto& [name, inv] : p_updated.inventories()) {
        InventoryContent& cached_inv { cached_state.inventories.at(name) };

        for (const auto& [item, qty] : inv.content()) {
            int& cached_item_qty { cached_inv.at(item) };

            if (!cache_initialized || qty != cached_item_qty) {
                update.items[name].insert({ item, qty }); // Utilisation de [] pour créer l'inventaire uniquement s'il y a des changements dessus
                cached_item_qty = qty;
            }
        }

        const InventorySize capacity { inv.capacity() };
        InventorySize& cached_capacity { cached_state.capacities.at(name) };
        if (!cache_initialized || capacity != cached_capacity) {
            update.capacities.insert({ name, capacity });
            cached_capacity = capacity;
        }
    }

    SessionDataFactory data_factory;
    data_factory.makePlayerUpdate(id, update);

    ctx_.sendToAll(data_factory.dataWithLength());

    cache_initialized = true;
}

void Gameplay::sendBattleInit(const GroupDescriptor& entities) {
    SessionDataFactory data_factory;
    data_factory.makeBattleInit(entities, game());

    ctx_.sendToAll(data_factory.dataWithLength());
}

void Gameplay::sendBattleAtk(const byte p_id, const std::string& enemy, const int dmg) {
    SessionDataFactory data_factory;
    data_factory.makeBattleAtk(p_id, enemy, dmg);

    ctx_.sendToAll(data_factory.dataWithLength());
}

void Gameplay::sendBattleEnd() {
    SessionDataFactory data_factory;
    data_factory.makeBattle(Battle::End);

    ctx_.sendToAll(data_factory.dataWithLength());
}

} // namespace Rbo
