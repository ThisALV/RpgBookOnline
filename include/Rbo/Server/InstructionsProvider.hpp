#ifndef INSTRUCTIONSPROVIDER_HPP
#define INSTRUCTIONSPROVIDER_HPP

#include <Rbo/Common.hpp>

#include <Rbo/Server/TablesLock.hpp>

namespace Rbo::Server {

struct UnknownInstruction : std::logic_error {
    std::string instruction;

    explicit UnknownInstruction(const std::string& name) : std::logic_error { "Unknown instruction \"" + name + '"' }, instruction { name } {}
};

class InstructionsProvider {
private:
    using LuaFunc = sol::function;
    using Instructions = std::unordered_map<std::string, LuaFunc>;

    struct LuaInstruction {
        LuaFunc func;
        sol::table args;

        Next operator()(Gameplay&) const;
    };

    Instructions instructions_;
    sol::state& ctx_;
    spdlog::logger& logger_;
    TablesLock resources_lock_;

    void initContainersAPI();
    void initGameAPI();
    void initGameplayAPI();

public:
    InstructionsProvider(sol::state& lua, spdlog::logger& logger);

    InstructionsProvider(const InstructionsProvider&) = delete;
    InstructionsProvider& operator=(const InstructionsProvider&) = delete;

    bool operator==(const InstructionsProvider&) const = delete;

    void load();
    Instruction get(const std::string& name, const sol::table args) const;
    bool has(const std::string& name) const { return instructions_.count(name) == 1; }
};

} // namespace Rbo::Server

#endif // INSTRUCTIONSPROVIDER_HPP
