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

std::string Gameplay::checkpoint(const std::string& chkpt, const word id) {
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

Replies Gameplay::askReply(const byte target, const std::string& msg, const byte min, const byte max, const bool wait) {
    SessionDataFactory data_factory;
    data_factory.makeRange(msg, min, max);

    return ctx_.request(target, data_factory.dataWithLength(), Controllers::RangeController { min, max }, wait);
}

Replies Gameplay::askReply(const byte target, const std::string& msg, const OptionsList& options, const bool wait) {
    SessionDataFactory data_factory;
    data_factory.makePossibilities(msg, options);

    std::vector<byte> ids;
    ids.resize(options.size());

    std::transform(options.cbegin(), options.cend(), ids.begin(), [](const auto& o) -> byte {
        return o.first;
    });

    return ctx_.request(target, data_factory.dataWithLength(), Controllers::PossibilitiesController { ids }, wait);
}

Replies Gameplay::askConfirm(const byte target, const bool wait) {
    SessionDataFactory data_factory;
    data_factory.makeRequest(Request::Confirm);

    return ctx_.request(target, data_factory.dataWithLength(), Controllers::confirmController, wait);
}

Replies Gameplay::askYesNo(const byte target, const std::string& question, const bool wait) {
    SessionDataFactory data_factory;
    data_factory.makeYesNoQuestion(question);

    return ctx_.request(target, data_factory.dataWithLength(), Controllers::RangeController { 0, 1 }, wait);
}

PlayerCheckingResult Gameplay::checkPlayer(const byte id) {
    const Player& p { player(id) } ;

    const std::vector<Condition>& conditions { game().deathConditions };
    const bool alive = std::none_of(conditions.cbegin(), conditions.cend(), [&p](const auto& c) -> bool {
        return c.test(p.stats());
    });

    if (alive)
        return { false, false, false };

    SessionDataFactory dead;
    dead.makeDie(id);

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
    const std::vector<Condition>& conditions { game().gameEndConditions };
    const bool g_continue = std::none_of(conditions.cbegin(), conditions.cend(), [this](const auto& c) -> bool {
        return c.test(global());
    });

    if (!g_continue)
        ctx_.stop();

    return g_continue;
}

void Gameplay::print(const std::string& txt, const byte target) {
    SessionDataFactory data_factory;
    data_factory.makeText(txt);

    ctx_.sendTo(target, data_factory.dataWithLength());
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
        capacities.insert({ name, inv.maxSize() });
    }

    players_cache_.insert({ player_id, PlayerState { target.stats().raw(), std::move(inventories), std::move(capacities) } });
}

void Gameplay::sendPlayerUpdate(const byte id) {
    Player& p_updated { player(id) };
    PlayerState& cache { players_cache_.at(id) };

    PlayerUpdate update;
    for (const auto& [name, stat] : p_updated.stats()) {
        Stat& cached_stat { cache.stats.at(name) };

        if (first_player_update_ || stat != cached_stat) {
            update.stats.insert({ name, stat });

            if (stat.hidden)
                cached_stat.hidden = true;
            else
                cached_stat = stat;
        }
    }

    for (const auto& [name, inv] : p_updated.inventories()) {
        InventoryContent& cached_inv { cache.inventories.at(name) };

        for (const auto& [item, qty] : inv.content()) {
            uint& cached_item_qty { cached_inv.at(item) };

            if (qty != cached_item_qty) {
                update.items[name].insert({ item, qty }); // Utilisation de [] pour créer l'inventaire uniquement s'il y a des changements dessus
                cached_item_qty = qty;
            }
        }

        const InventorySize capacity { inv.maxSize() };
        InventorySize& cached_capacity { cache.capacities.at(name) };
        if (capacity != cached_capacity) {
            update.capacities.insert({ name, capacity });
            cached_capacity = capacity;
        }
    }

    SessionDataFactory data_factory;
    data_factory.makePlayerUpdate(id, update);

    ctx_.sendToAll(data_factory.dataWithLength());

    first_player_update_ = false;
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
