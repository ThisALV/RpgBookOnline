#include "InstructionsProvider.hpp"

#include "spdlog/logger.h"
#include "Gameplay.hpp"
#include "Game.hpp"
#include "Player.hpp"

bool isInstruction(const sol::object& key, const sol::object& value) {
    return key.get_type() == sol::type::string && value.get_type() == sol::type::function;
}

Next InstructionsProvider::LuaInstruction::operator()(Gameplay& interface) const {
    const sol::function_result result { func(interface, sol::as_container(args)) };

    if (!result.valid()) {
        const sol::error err { result };
        if (std::string { err.what() } == "CancelledRequest")
            return {};

        throw err;
    }

    if (!provider->validLuaResources())
        throw IllegalLuaModifications {};

    const sol::optional<Next> next { result };
    return next ? *next : Next {};
}

const std::vector<std::string> usertypes {
    "Gameplay", "Player", "EventEffect", "Game", "StatsManager", "RestProperties", "SimulationResult"
};

InstructionsProvider::InstructionsProvider(sol::state& lua, spdlog::logger& logger)
    : lua_ { std::move(lua) }, logger_ { &logger }
{
    initUsertypes();
    registerBuiltinFunc("vote", vote);
    lua_.create_named_table("errorHandlers");

    for (const std::string& type : usertypes)
        usertypes_.insert({ type, lua_[type] });

    const auto& rbo { lua_["Rbo"] };
    if (rbo.get_type() != sol::type::table)
        return;

    for (const auto& [key, value] : rbo.get<sol::table>()) {
        if (!isInstruction(key, value))
            continue;

        const std::string name { key.as<std::string>() };
        logger_->debug("Ajout de {}.", name);

        sol::function instruction { value.as<sol::function>() };
        lua_["errorHandlers"][name] = [name](const std::string& err) -> std::string {
            return err == "CancelledRequest" ? err : name + " : " + err;
        };
        instruction.error_handler = lua_["errorHandlers"][name];

        instructions_.insert({ name, instruction });
    }

    error_handlers_ = errorHandlers();
}

template<typename Container> sol::as_container_t<Container> luaContainer(Container& c) {
    return sol::as_container(c);
}

template<typename T> sol::as_container_t<std::vector<T>> luaVector(std::vector<T>& v) {
    return luaContainer<std::vector<T>>(v);
}

void InstructionsProvider::initUsertypes() {
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

bool InstructionsProvider::validLuaResources() const {
    return std::all_of(usertypes.cbegin(), usertypes.cend(), [this](const std::string& type) {
        return usertypes_.at(type) == lua_[type];
    }) && std::all_of(builtin_funcs_.cbegin(), builtin_funcs_.cend(), [this](const auto& f) {
        return f.second == lua_[f.first];
    }) && lua_["errorHandlers"].get_type() == sol::type::table && error_handlers_ == errorHandlers();
}

Instruction InstructionsProvider::get(const std::string& name,
                                      const std::vector<std::string>& args) const
{
    if (instructions_.count(name) == 0)
        throw UnknownInstruction { name };

    return LuaInstruction { this, instructions_.at(name), args };
}

std::unordered_map<std::string, sol::object> InstructionsProvider::errorHandlers() const {
    assert(lua_["errorHandlers"].get_type() == sol::type::table);

    const sol::table error_handlers { lua_["errorHandlers"].get<sol::table>() };

    std::unordered_map<std::string, sol::object> handlers;
    std::transform(error_handlers.cbegin(), error_handlers.cend(),
                   std::inserter(handlers, handlers.begin()),
                   [](const std::pair<sol::object, sol::object>& h)
    {
        if (h.first.get_type() != sol::type::string)
            throw IllegalLuaModifications {};

        return std::pair<std::string, sol::object> { h.first.as<std::string>(), h.second };
    });

    return handlers;
}
