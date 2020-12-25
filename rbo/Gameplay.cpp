#include <Rbo/Gameplay.hpp>

#include <Rbo/Session.hpp>
#include <Rbo/SessionDataFactory.hpp>

namespace Rbo {

namespace Controllers {

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

struct PossibilitiesController {
    const std::vector<byte>& possibilities;

    void operator()(const byte reply) const {
        const auto cend { possibilities.cend() };

        if (std::find(possibilities.cbegin(), cend, reply) == cend)
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

OptionsList Gameplay::names() const {
    const Players players { ctx_.players() };

    OptionsList names;
    std::transform(players.cbegin(), players.cend(), std::inserter(names, names.begin()), [](const auto p) {
        return OptionsList::value_type { p.first, p.second->name() };
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

void Gameplay::voteForLeader() {
    switchLeader(vote(askReply(ALL_PLAYERS, "Qui doit devenir leader ?", names(), true)));
}

Replies Gameplay::askReply(const byte target, const std::string& msg, const byte min, const byte max, const bool first_reply_only, const bool wait_all_replies) {
    SessionDataFactory data_factory;
    data_factory.makeRange(target, msg, min, max);

    return ctx_.request(target, data_factory.dataWithLength(), Controllers::RangeController { min, max }, first_reply_only, wait_all_replies);
}

Replies Gameplay::askReply(const byte target, const std::string& msg, const OptionsList& options, const bool first_reply_only, const bool wait_all_replies) {
    SessionDataFactory data_factory;
    data_factory.makePossibilities(target, msg, options);

    std::vector<byte> ids;
    ids.resize(options.size());

    std::transform(options.cbegin(), options.cend(), ids.begin(), [](const auto& o) -> byte {
        return o.first;
    });

    return ctx_.request(target, data_factory.dataWithLength(), Controllers::PossibilitiesController { ids }, first_reply_only, wait_all_replies);
}

Replies Gameplay::askConfirm(const byte target, const bool first_reply_only) {
    SessionDataFactory data_factory;
    data_factory.makeRequest(Request::Confirm, target);

    // Il n'y a aucun intérêt à ne pas prendre en compte une confirmation reçue car elle serait arrivée trop tard
    return ctx_.request(target, data_factory.dataWithLength(), Controllers::confirmController, first_reply_only, false);
}

Replies Gameplay::askYesNo(const byte target, const std::string& question, const bool first_reply_only, const bool wait_all_replies) {
    SessionDataFactory data_factory;
    data_factory.makeYesNoQuestion(target, question);

    return ctx_.request(target, data_factory.dataWithLength(), Controllers::RangeController { 0, 1 }, first_reply_only, wait_all_replies);
}

Replies Gameplay::askDiceRoll(const byte target, const std::string& msg, const DicesRoll& formula, const DiceRollResults& results) {
    if (target != ALL_PLAYERS && results.count(target) == 0)
        throw InvalidDiceRollResults { target };

    SessionDataFactory data_factory;
    data_factory.makeDiceRoll(target, msg, formula.dices, formula.bonus, results);

    return ctx_.request(target, data_factory.dataWithLength(), Controllers::confirmController, false, true);
}

PlayerCheckingResult Gameplay::checkPlayer(const byte id) {
    const Player& p { player(id) } ;

    const std::vector<DeathCondition>& conditions { game().deathConditions };
    const auto death = std::find_if(conditions.cbegin(), conditions.cend(), [&p](const auto& dc) {
        return dc.dieIf.test(p.stats());
    });

    const bool alive { death == conditions.cend() };

    if (alive)
        return { false, false, false };

    SessionDataFactory dead;
    dead.makeDie(id, death->deathMessage);

    ctx_.sendToAll(dead.dataWithLength());
    ctx_.disconnect(id);

    const bool end { !ctx_.playersRemaining() };
    if (end)
        ctx_.stop();

    const bool leader_switch { leader() == id };
    if (leader_switch)
        switchLeader(*players().cbegin());

    return { true, leader_switch, end };
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

    players_cache_.insert({ player_id, PlayerCache { false,  PlayerState { target.stats().raw(), std::move(inventories), std::move(capacities) } } });
}

void Gameplay::sendPlayerUpdate(const byte id) {
    Player& p_updated { player(id) };
    PlayerCache& cache { players_cache_.at(id) };
    PlayerState& cached_state { cache.playerState };
    bool& cache_initialized { cache.initialized };

    PlayerUpdate update;
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
