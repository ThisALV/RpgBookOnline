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

    const sol::optional<Next> next { result };
    return next ? *next : Next {};
}

InstructionsProvider::InstructionsProvider(sol::state& ctx, spdlog::logger& logger)
    : ctx_ { ctx }, logger_ { logger }, resources_lock_ { ctx }
{
    ctx_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine, sol::lib::string,
                        sol::lib::math, sol::lib::table);

    sol::table global { ctx_.globals() };
    for (const auto [key, value] : global) {
        if (value.get_type() == sol::type::table && value != global && value != ctx_[sol::env_key])
            resources_lock_(global[key]);
    }

    ctx_.create_named_table("Rbo");
    ctx_.create_named_table("ErrorHandlers");

    initUsertypes();
    ctx_["getIDs"] = getIDs;
    ctx_["reply"] = reply;
    ctx_["vote"] = vote;
    ctx_["allPlayers"] = 0;
    ctx_["setmetatable"] = sol::nil;
    ctx_["getmetatable"] = sol::nil;
    ctx_["print"] = sol::nil;

    resources_lock_(global);
}

void InstructionsProvider::load() {
    sol::table global { resources_lock_.get(ctx_.globals()).as<sol::table>() };
    sol::table error_handlers { global["ErrorHandlers"] };
    sol::table rbo { global["Rbo"] };
    for (const auto [key, value] : rbo) {
        if (!isInstruction(key, value))
            continue;

        const std::string name { key.as<std::string>() };
        logger_.debug("Ajout de {}.", name);

        sol::function instruction { value.as<sol::function>() };
        error_handlers[name] = [name](const std::string& err) -> std::string {
            return err == "CancelledRequest" ? err : name + " : " + err;
        };
        instruction.error_handler = error_handlers[name];

        instructions_.insert({ name, instruction });
    }

    resources_lock_(error_handlers);
    resources_lock_(rbo);
}

Instruction InstructionsProvider::get(const std::string& name,
                                      const std::vector<std::string>& args) const
{
    if (instructions_.count(name) == 0)
        throw UnknownInstruction { name };

    return LuaInstruction { instructions_.at(name), args };
}

} // namespace Rbo::Server
