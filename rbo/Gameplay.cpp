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
    const Players players { ctx_.players() };
    std::vector<byte> ids;
    ids.resize(ctx_.count(), 0);

    std::transform(players.cbegin(), players.cend(), ids.begin(), [](const auto p) {
        return p.first;
    });

    return ids;
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
    const Players players { ctx_.players() };

    std::vector<byte> ids;
    ids.resize(players.size(), 0);

    std::transform(players.cbegin(), players.cend(), ids.begin(), [](const auto p) -> byte {
        return p.first;
    });

    switchLeader(vote(askReply(ALL_PLAYERS, ids, true)));
}

Replies Gameplay::askReply(const byte target, const byte min, const byte max, const bool wait) {
    SessionDataFactory data_factory;
    data_factory.makeRange(min, max);

    return ctx_.request(target, data_factory.dataWithLength(), Controllers::RangeController { min, max }, wait);
}

Replies Gameplay::askReply(const byte target, const std::vector<byte>& range, const bool wait) {
    SessionDataFactory data_factory;
    data_factory.makePossibilities(range);

    return ctx_.request(target, data_factory.dataWithLength(), Controllers::PossibilitiesController { range }, wait);
}

Replies Gameplay::askConfirm(const byte target, const bool wait) {
    SessionDataFactory data_factory;
    data_factory.makeRequest(Request::Confirm);

    return ctx_.request(target, data_factory.dataWithLength(), Controllers::confirmController, wait);
}

Replies Gameplay::askYesNo(const byte target, const bool wait) {
    SessionDataFactory data_factory;
    data_factory.makeRequest(Request::YesNo);

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
    data_factory.makePlain(txt);

    ctx_.sendTo(target, data_factory.dataWithLength());
}

void Gameplay::printOptions(const OptionsList& options, const byte target) {
    SessionDataFactory data_factory;
    data_factory.makeOptions(options);

    ctx_.sendTo(target, data_factory.dataWithLength());
}

void Gameplay::printOptions(const std::vector<std::string>& options, const byte target, const byte begin) {
    OptionsList list;
    std::size_t i { begin };
    for (const std::string& option : options)
        list.insert({ i++, option });

    SessionDataFactory data_factory;
    data_factory.makeOptions(list);

    ctx_.sendTo(target, data_factory.dataWithLength());
}

const std::vector<std::string> yes_no { "Oui", "Non" };

void Gameplay::printYesNo(const byte target) {
    printOptions(yes_no, target);
}

void Gameplay::sendGlobalStat(const std::string& stat) {
    const auto [min, max] { global().limits(stat) };

    SessionDataFactory data_factory;
    data_factory.makeGlobalStat(stat, min, max, global().get(stat), global().hidden(stat));

    ctx_.sendToAll(data_factory.dataWithLength());
}

void Gameplay::sendInfos(const byte id) {
    SessionDataFactory data_factory;
    data_factory.makeInfos(id, ctx_.getChanges(id));

    ctx_.sendToAll(data_factory.dataWithLength());
}

void Gameplay::sendBattleInfos(const GroupDescriptor& entities) {
    SessionDataFactory data_factory;
    data_factory.makeBattleInit(entities, game().enemies);

    ctx_.sendToAll(data_factory.dataWithLength());
}

void Gameplay::sendBattleAtk(const byte p_id, const std::string& enemy, const int dmg) {
    SessionDataFactory data_factory;
    data_factory.makeAtk(p_id, enemy, dmg);

    ctx_.sendToAll(data_factory.dataWithLength());
}

void Gameplay::endBattle() {
    SessionDataFactory data_factory;
    data_factory.makeBattleInfos(BattleInfo::End);

    ctx_.sendToAll(data_factory.dataWithLength());
}

} // namespace Rbo
