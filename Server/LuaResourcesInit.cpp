#include "LocalGameBuilder.hpp"

#include "Gameplay.hpp"
#include "Player.hpp"
#include "Game.hpp"

template<typename Container> sol::as_container_t<Container> luaContainer(Container& c) {
    return sol::as_container(c);
}

template<typename T> sol::as_container_t<std::vector<T>> luaVector(std::vector<T>& v) {
    return luaContainer<std::vector<T>>(v);
}

void InstructionsProvider::initLuaResources() {
    lua_.new_usertype<std::vector<std::string>>(
                "StringVector", sol::constructors<std::vector<std::string>()>(),
                "iterable", luaVector<std::string>);
    lua_.new_usertype<std::vector<byte>>(
                "ByteVector", sol::constructors<std::vector<byte>()>(),
                "iterable", luaVector<byte>);

    lua_.new_usertype<std::unordered_map<byte, std::string>>(
                "ByteWithString", sol::constructors<std::unordered_map<byte, std::string>()>(),
                "iterable", luaContainer<std::unordered_map<byte, std::string>>);
    lua_.new_usertype<std::unordered_map<byte, std::string>>(
                "StringWithString", sol::constructors<std::unordered_map<std::string, std::string>()>(),
                "iterable", luaContainer<std::unordered_map<std::string, std::string>>);

    sol::usertype<RestProperties> rest_type { lua_.new_usertype<RestProperties>("RestProperties") };
    rest_type["givables"] = sol::readonly(&RestProperties::givables);
    rest_type["availables"] = sol::readonly(&RestProperties::availables);

    sol::usertype<StatLimits> limits_type { lua_.new_usertype<StatLimits>("StatLimits") };
    limits_type["min"] = &StatLimits::min;
    limits_type["max"] = &StatLimits::max;

    sol::usertype<StatsManager> stats_type { lua_.new_usertype<StatsManager>("StatsManager") };
    stats_type["get"] = &StatsManager::get;
    stats_type["change"] = &StatsManager::change;
    stats_type["set"] = &StatsManager::set;
    stats_type["setLimits"] = &StatsManager::setLimits;
    stats_type["limits"] = &StatsManager::limits;
    stats_type["setHidden"] = &StatsManager::setHidden;
    stats_type["hidden"] = &StatsManager::hidden;
    stats_type["has"] = &StatsManager::has;

    sol::usertype<Inventory> inv_type { lua_.new_usertype<Inventory>("Inventory") };
    inv_type["add"] = &Inventory::add;
    inv_type["consume"] = &Inventory::consume;
    inv_type["size"] = &Inventory::size;
    inv_type["count"] = &Inventory::count;
    inv_type["has"] = &Inventory::has;
    inv_type["limited"] = &Inventory::limited;
    inv_type["maxSize"] = &Inventory::maxSize;
    inv_type["setMaxSize"] = &Inventory::setMaxSize;

    sol::usertype<Player> player_type { lua_.new_usertype<Player>("Player") };
    player_type["same"] = &Player::same;
    player_type["id"] = &Player::id;
    player_type["name"] = &Player::name;
    player_type["add"] = &Player::add;
    player_type["consume"] = &Player::consume;
    player_type["inventory"] = static_cast<Inventory&(Player::*)(const std::string&)>(&Player::inventory);
    player_type["stats"] = static_cast<StatsManager&(Player::*)()>(&Player::stats);

    const std::initializer_list<std::pair<std::string_view, EventEffect::ItemsChanges>> results {
        { "Ok", EventEffect::ItemsChanges::Ok },
        { "InventoryFull", EventEffect::ItemsChanges::InvFull },
        { "ItemEmpty", EventEffect::ItemsChanges::ItemEmpty }
    };
    lua_.new_enum<EventEffect::ItemsChanges>("SimulationResult", results);

    sol::usertype<EventEffect> effect_type { lua_.new_usertype<EventEffect>("EventEffect") };
    effect_type["apply"] = &EventEffect::apply;
    effect_type["simulateItemsChanges"] = &EventEffect::simulateItemsChanges;

    sol::usertype<Game> game_type { lua_.new_usertype<Game>("Game") };
    game_type["name"] = sol::readonly(&Game::name);
    game_type["voteOnLeaderDeath"] = sol::readonly(&Game::voteOnLeaderDeath);
    game_type["voteLeader"] = sol::readonly(&Game::voteLeader);
    game_type["rest"] = sol::readonly(&Game::rest);
    game_type["effect"] = &Game::effect;

    sol::usertype<PlayerCheckingResult> check_result_type {
        lua_.new_usertype<PlayerCheckingResult>("CheckingResult")
    };
    check_result_type["alive"] = sol::readonly(&PlayerCheckingResult::alive);
    check_result_type["leaderSwitch"] = sol::readonly(&PlayerCheckingResult::leaderSwitch);
    check_result_type["sessionEnd"] = sol::readonly(&PlayerCheckingResult::sessionEnd);

    sol::usertype<Gameplay> gameplay_type { lua_.new_usertype<Gameplay>("Gameplay") };
    gameplay_type["global"] = static_cast<StatsManager&(Gameplay::*)()>(&Gameplay::global);
    gameplay_type["game"] = [](Gameplay& ctx) { return Game { ctx.game() }; };
    gameplay_type["rest"] = [](Gameplay& ctx) { return RestProperties { ctx.rest() }; };
    gameplay_type["checkpoint"] = &Gameplay::checkpoint;
    gameplay_type["player"] = static_cast<Player&(Gameplay::*)(const byte)>(&Gameplay::player);
    gameplay_type["count"] = &Gameplay::count;
    gameplay_type["leader"] = &Gameplay::leader;
    gameplay_type["switchLeader"] = &Gameplay::switchLeader;
    gameplay_type["voteForLeader"] = &Gameplay::voteForLeader;
    gameplay_type["askReply"] = sol::overload(
        static_cast<Replies(Gameplay::*)(const byte, const byte, const byte, const bool)>(&Gameplay::askReply),
        static_cast<Replies(Gameplay::*)(const byte, const std::vector<byte>&, const bool)>(&Gameplay::askReply)
    );
    gameplay_type["askConfirm"] = &Gameplay::askConfirm;
    gameplay_type["askYesNo"] = &Gameplay::askYesNo;
    gameplay_type["checkPlayer"] = &Gameplay::checkPlayer;
    gameplay_type["checkGame"] = &Gameplay::checkGame;
    gameplay_type["print"] = &Gameplay::print;
    gameplay_type["printOptions"] = sol::overload(
        static_cast<void(Gameplay::*)(const OptionsList&, const byte)>(&Gameplay::printOptions),
        static_cast<void(Gameplay::*)(const std::vector<std::string>&, const byte, const byte)>(&Gameplay::printOptions)
    );
    gameplay_type["printYesNo"] = &Gameplay::printYesNo;
    gameplay_type["sendGlobalStat"] = &Gameplay::sendGlobalStat;
    gameplay_type["sendInfos"] = &Gameplay::sendInfos;
    gameplay_type["sendBattleInfos"] = &Gameplay::sendBattleInfos;
    gameplay_type["sendBattleAtk"] = &Gameplay::sendBattleAtk;
    gameplay_type["endBattle"] = &Gameplay::endBattle;
}
