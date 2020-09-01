#include "InstructionsProvider.hpp"

#include "Gameplay.hpp"
#include "Player.hpp"
#include "Game.hpp"
#include "Enemy.hpp"

namespace Rbo::Server {

namespace AsContainer {

template<typename Container> sol::as_container_t<Container> luaContainer(Container& c) {
    return sol::as_container(c);
}

template<typename T> sol::as_container_t<std::vector<T>> luaVector(std::vector<T>& v) {
    return luaContainer<std::vector<T>>(v);
}

} // namespace AsContainer

std::vector<byte> InstructionsProvider::getIDs(const Replies& replies) {
    std::vector<byte> ids;
    ids.resize(replies.size(), 0);
    std::transform(replies.cbegin(), replies.cend(), ids.begin(),
                   [](const auto r) { return r.first; });

    return ids;
}

std::tuple<byte, byte> InstructionsProvider::reply(const Replies& replies) {
    if (replies.size() != 1)
        throw std::invalid_argument { "Pour récupérer une réponse unique, il faut 1 réponse" };

    const auto reply { replies.cbegin() };
    return std::make_tuple(reply->first, reply->second);
}

void InstructionsProvider::assertArgs(const bool assertion) {
    if (!assertion)
        throw std::invalid_argument { "Arguments invalides" };
}

bool InstructionsProvider::toBoolean(const std::string& str) {
    if (str == "true")
        return true;
    else if (str == "false")
        return false;

    throw std::invalid_argument { "Convertion de \"" + str + "\" en booléen impossible" };
}

void InstructionsProvider::applyToGlobal(Gameplay& ctx, const EventEffect& effect) {
    if (!effect.itemsChanges.empty())
        throw std::logic_error { "La session ne possède pas d'inventaires ni d'objets" };

    for (const auto& [stat, change] : effect.statsChanges) {
        ctx.global().change(stat, change);

        if (!ctx.global().hidden(stat))
            ctx.sendGlobalStat(stat);
    }
}

void InstructionsProvider::initBuiltins() {
    using namespace AsContainer;

    ctx_.new_usertype<std::vector<std::string>>(
                "StringVector", sol::constructors<std::vector<std::string>()>(),
                "iterable", luaVector<std::string>);
    ctx_.new_usertype<std::vector<byte>>(
                "ByteVector", sol::constructors<std::vector<byte>()>(),
                "iterable", luaVector<byte>);

    ctx_.new_usertype<std::map<byte, std::string>>(
                "ByteWithString", sol::constructors<std::map<byte, std::string>()>(),
                "iterable", luaContainer<std::map<byte, std::string>>);
    ctx_.new_usertype<std::unordered_map<std::string, std::string>>(
                "StringWithString", sol::constructors<std::unordered_map<std::string, std::string>()>(),
                "iterable", luaContainer<std::unordered_map<std::string, std::string>>);
    ctx_.new_usertype<Effects>(
                "Effects", sol::constructors<Effects()>(),
                "iterable", luaContainer<Effects>);
    ctx_.new_usertype<EnemiesGroup>(
                "Group", sol::constructors<EnemiesGroup()>(),
                "iterable", luaContainer<EnemiesGroup>);

    sol::usertype<RestProperties> rest_type { ctx_.new_usertype<RestProperties>("RestProperties") };
    rest_type["givables"] = sol::readonly(&RestProperties::givables);
    rest_type["availables"] = sol::readonly(&RestProperties::availables);

    sol::usertype<StatLimits> limits_type { ctx_.new_usertype<StatLimits>("StatLimits") };
    limits_type["min"] = &StatLimits::min;
    limits_type["max"] = &StatLimits::max;

    sol::usertype<StatsManager> stats_type { ctx_.new_usertype<StatsManager>("StatsManager") };
    stats_type["get"] = &StatsManager::get;
    stats_type["change"] = &StatsManager::change;
    stats_type["set"] = &StatsManager::set;
    stats_type["setLimits"] = &StatsManager::setLimits;
    stats_type["limits"] = &StatsManager::limits;
    stats_type["setHidden"] = &StatsManager::setHidden;
    stats_type["hidden"] = &StatsManager::hidden;
    stats_type["has"] = &StatsManager::has;

    sol::usertype<Inventory> inv_type { ctx_.new_usertype<Inventory>("Inventory") };
    inv_type["add"] = &Inventory::add;
    inv_type["consume"] = &Inventory::consume;
    inv_type["size"] = &Inventory::size;
    inv_type["count"] = &Inventory::count;
    inv_type["has"] = &Inventory::has;
    inv_type["limited"] = &Inventory::limited;
    inv_type["maxSize"] = &Inventory::maxSize;
    inv_type["setMaxSize"] = &Inventory::setMaxSize;

    sol::usertype<Player> player_type { ctx_.new_usertype<Player>("Player") };
    player_type["same"] = &Player::same;
    player_type["id"] = &Player::id;
    player_type["name"] = &Player::name;
    player_type["add"] = &Player::add;
    player_type["consume"] = &Player::consume;
    player_type["inventory"] = sol::resolve<Inventory&(const std::string&)>(&Player::inventory);
    player_type["stats"] = sol::resolve<StatsManager&()>(&Player::stats);

    const std::initializer_list<std::pair<std::string_view, EventEffect::ItemsChanges>> results {
        { "Ok", EventEffect::ItemsChanges::Ok },
        { "InventoryFull", EventEffect::ItemsChanges::InvFull },
        { "ItemEmpty", EventEffect::ItemsChanges::ItemEmpty }
    };
    ctx_.new_enum<EventEffect::ItemsChanges>("SimulationResult", results);

    sol::usertype<EventEffect> effect_type { ctx_.new_usertype<EventEffect>("EventEffect") };
    effect_type["apply"] = &EventEffect::apply;
    effect_type["simulateItemsChanges"] = &EventEffect::simulateItemsChanges;

    sol::usertype<Enemy> enemy_type { ctx_.new_usertype<Enemy>("Enemy", "create", &Enemy::create) };
    enemy_type["alive"] = &Enemy::alive;
    enemy_type["hp"] = &Enemy::hp;
    enemy_type["skill"] = &Enemy::skill;
    enemy_type["hit"] = &Enemy::hit;
    enemy_type["heal"] = &Enemy::heal;
    enemy_type["buff"] = &Enemy::buff;
    enemy_type["unbuff"] = &Enemy::unbuff;

    sol::usertype<Game> game_type { ctx_.new_usertype<Game>("Game") };
    game_type["name"] = sol::readonly(&Game::name);
    game_type["voteOnLeaderDeath"] = sol::readonly(&Game::voteOnLeaderDeath);
    game_type["voteLeader"] = sol::readonly(&Game::voteLeader);
    game_type["rest"] = sol::readonly(&Game::rest);
    game_type["effect"] = &Game::effect;
    game_type["enemy"] = &Game::enemy;
    game_type["group"] = &Game::group;

    sol::usertype<PlayerCheckingResult> check_result_type {
        ctx_.new_usertype<PlayerCheckingResult>("CheckingResult")
    };
    check_result_type["alive"] = sol::readonly(&PlayerCheckingResult::alive);
    check_result_type["leaderSwitch"] = sol::readonly(&PlayerCheckingResult::leaderSwitch);
    check_result_type["sessionEnd"] = sol::readonly(&PlayerCheckingResult::sessionEnd);

    sol::usertype<Gameplay> gameplay_type { ctx_.new_usertype<Gameplay>("Gameplay") };
    gameplay_type["global"] = &Gameplay::global;
    gameplay_type["game"] = &Gameplay::game;
    gameplay_type["rest"] = &Gameplay::rest;
    gameplay_type["checkpoint"] = &Gameplay::checkpoint;
    gameplay_type["player"] = &Gameplay::player;
    gameplay_type["players"] = &Gameplay::players;
    gameplay_type["count"] = &Gameplay::count;
    gameplay_type["leader"] = &Gameplay::leader;
    gameplay_type["switchLeader"] = &Gameplay::switchLeader;
    gameplay_type["voteForLeader"] = &Gameplay::voteForLeader;
    gameplay_type["askReply"] = sol::overload(
        [](Gameplay& ctx, const byte target, const byte min, const byte max) {
            return ctx.askReply(target, min, max);
        },
        [](Gameplay& ctx, const byte target, const std::vector<byte>& possibilities) {
            return ctx.askReply(target, possibilities);
        },
        sol::resolve<Replies(const byte, const byte, const byte, const bool)>(&Gameplay::askReply),
        sol::resolve<Replies(const byte, const std::vector<byte>&, const bool)>(&Gameplay::askReply)
    );
    gameplay_type["askConfirm"] = sol::overload(
        [](Gameplay& ctx, const byte target) { return ctx.askConfirm(target); },
        &Gameplay::askConfirm
    );
    gameplay_type["askYesNo"] = sol::overload(
        [](Gameplay& ctx, const byte target) { return ctx.askYesNo(target); },
        &Gameplay::askYesNo
    );
    gameplay_type["checkPlayer"] = &Gameplay::checkPlayer;
    gameplay_type["checkGame"] = &Gameplay::checkGame;
    gameplay_type["print"] = sol::overload(
        [](Gameplay& ctx, const std::string& text) { ctx.print(text); }, &Gameplay::print
    );
    gameplay_type["printOptions"] = sol::overload(
        [](Gameplay& ctx, const OptionsList& list) { ctx.printOptions(list); },
        [](Gameplay& ctx, const std::vector<std::string>& list) { ctx.printOptions(list); },
        sol::resolve<void(const OptionsList&, const byte)>(&Gameplay::printOptions),
        sol::resolve<void(const std::vector<std::string>&, const byte, const byte)>(&Gameplay::printOptions)
    );
    gameplay_type["printYesNo"] = sol::overload(
        [](Gameplay& ctx) { ctx.printYesNo(); }, &Gameplay::printYesNo
    );
    gameplay_type["sendGlobalStat"] = &Gameplay::sendGlobalStat;
    gameplay_type["sendInfos"] = &Gameplay::sendInfos;
    gameplay_type["sendBattleInfos"] = &Gameplay::sendBattleInfos;
    gameplay_type["sendBattleAtk"] = &Gameplay::sendBattleAtk;
    gameplay_type["sendBattleEnd"] = &Gameplay::endBattle;

    ctx_["getIDs"] = getIDs;
    ctx_["reply"] = reply;
    ctx_["vote"] = vote;
    ctx_["namesOf"] = namesOf;
    ctx_["assertArgs"] = assertArgs;
    ctx_["toBoolean"] = toBoolean;
    ctx_["applyToGlobal"] = applyToGlobal;
    ctx_["ALL_PLAYERS"] = 0;
    ctx_["setmetatable"] = sol::nil;
    ctx_["getmetatable"] = sol::nil;
#ifdef NDEBUG
    ctx_["print"] = sol::nil;
#endif
}

} // namespace Rbo::Server
