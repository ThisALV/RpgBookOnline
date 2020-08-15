#include "InstructionsProvider.hpp"

#include "spdlog/logger.h"
#include "Gameplay.hpp"

namespace Rbo::Server {

std::vector<byte> getIDs(const Replies& replies) {
    std::vector<byte> ids;
    ids.resize(replies.size(), 0);
    std::transform(replies.cbegin(), replies.cend(), ids.begin(),
                   [](const auto r) { return r.first; });

    return ids;
}

std::tuple<byte, byte> reply(const Replies& replies) {
    if (replies.size() != 1)
        throw std::invalid_argument { "Pour récupérer une réponse unique, il faut 1 réponse" };

    const auto reply { replies.cbegin() };
    return std::make_tuple(reply->first, reply->second);
}

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
    registerBuiltinVar("getIDs", getIDs);
    registerBuiltinVar("reply", reply);
    registerBuiltinVar("vote", vote);
    registerBuiltinVar("allPlayers", 0);
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

bool InstructionsProvider::validLuaResources() const {
    return std::all_of(usertypes.cbegin(), usertypes.cend(), [this](const std::string& type) {
        return usertypes_.at(type) == lua_[type];
    }) && std::all_of(builtin_vars_.cbegin(), builtin_vars_.cend(), [this](const auto& v) {
        return v.second == lua_[v.first];
    }) && std::all_of(instructions_.cbegin(), instructions_.cend(), [this](const auto& f) {
        return f.second == lua_["Rbo"][f.first];
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

} // namespace Rbo::Server
