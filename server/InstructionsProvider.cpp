#include <Rbo/Server/InstructionsProvider.hpp>

#include <spdlog/logger.h>
#include <Rbo/Gameplay.hpp>
#include <Rbo/Enemy.hpp>

namespace Rbo::Server {

namespace {

bool isInstruction(const sol::object& key, const sol::object& value) {
    return key.get_type() == sol::type::string && value.get_type() == sol::type::function;
}

}

Next InstructionsProvider::LuaInstruction::operator()(Gameplay& interface) const {
    const sol::function_result result { func(interface, args) };

    if (!result.valid()) {
        const sol::error err { result.get<sol::error>() };
        if (std::string { err.what() } == "CanceledRequest")
            return {};

        throw err;
    }

    const auto next { result.get<sol::optional<Next>>() };
    return next ? *next : Next {};
}

InstructionsProvider::InstructionsProvider(sol::state& ctx, spdlog::logger& logger) : ctx_ { ctx }, logger_ { logger }, resources_lock_ { ctx } {
    ctx_.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine, sol::lib::string, sol::lib::math, sol::lib::table);

    sol::table global { ctx_.globals() };
    for (const auto& [key, value] : global) {
        if (value.get_type() == sol::type::table && value != global && value != ctx_[sol::env_key])
            resources_lock_(global[key]);
    }

    initContainersAPI();
    initGameAPI();
    initGameplayAPI();

    ctx_.create_named_table("Rbo");
    ctx_.create_named_table("ErrorHandlers");

    resources_lock_(global);
}

void InstructionsProvider::load() {
    sol::table global { resources_lock_.get(ctx_.globals()).as<sol::table>() };
    sol::table error_handlers { global["ErrorHandlers"].get<sol::table>() };
    sol::table rbo { global["Rbo"].get<sol::table>() };
    for (const auto& [key, value] : rbo) {
        if (!isInstruction(key, value))
            continue;

        const std::string name { key.as<std::string>() };
        logger_.debug("Loading \"{}\"...", name);

        sol::function instruction { value.as<sol::function>() };
        error_handlers[name] = [name](const std::string& err) -> std::string {
            return err == "CanceledRequest" ? err : name + " : " + err;
        };
        instruction.error_handler = error_handlers[name];

        instructions_.insert({ name, instruction });
    }

    resources_lock_(error_handlers);
    resources_lock_(rbo);
}

Instruction InstructionsProvider::get(const std::string& name, const sol::table& args) const {
    if (!has(name))
        throw UnknownInstruction { name };

    return LuaInstruction { instructions_.at(name), args };
}

} // namespace Rbo::Server
