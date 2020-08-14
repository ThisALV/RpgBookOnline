#ifndef INSTRUCTIONSPROVIDER_HPP
#define INSTRUCTIONSPROVIDER_HPP

#include "Common.hpp"

#include "sol/sol.hpp"

struct UnknownInstruction : std::logic_error {
    std::string instruction;

    UnknownInstruction(const std::string& name)
        : std::logic_error { "Instruction \"" + name + "\" inconnue" }, instruction { name } {}
};

struct InstructionFailed : std::runtime_error {
    std::string instruction;

    InstructionFailed(const std::string& name, const std::string& msg)
        : std::runtime_error { name + " : " + msg }, instruction { name } {}
};

struct IllegalLuaModifications : std::runtime_error {
    IllegalLuaModifications() : std::runtime_error { "Ressources Lua illégalement modifiées" } {}
};

class InstructionsProvider {
private:
    using LuaFunc = sol::function;
    using Instructions = std::unordered_map<std::string, LuaFunc>;

    struct LuaInstruction {
        const InstructionsProvider* provider;
        LuaFunc func;
        std::vector<std::string> args;

        Next operator()(Gameplay&) const;
    };

    sol::state lua_;
    Instructions instructions_;
    spdlog::logger* logger_;

    std::unordered_map<std::string, sol::table> usertypes_;
    std::unordered_map<std::string, sol::object> error_handlers_;
    std::unordered_map<std::string, sol::function> builtin_funcs_;

    template<typename Func>
    void registerBuiltinFunc(const std::string& name, const Func& func) {
        lua_[name] = func;
        builtin_funcs_[name] = lua_[name];
    }
    void initLuaResources();

    std::unordered_map<std::string, sol::object> errorHandlers() const;
    bool validLuaResources() const;

public:
    InstructionsProvider() = default;
    InstructionsProvider(sol::state&, spdlog::logger&);

    InstructionsProvider(const InstructionsProvider&) = delete;
    InstructionsProvider& operator=(const InstructionsProvider&) = delete;

    InstructionsProvider(InstructionsProvider&&) = default;
    InstructionsProvider& operator=(InstructionsProvider&&) = default;

    bool operator==(const InstructionsProvider&) const = delete;

    Instruction get(const std::string&, const std::vector<std::string>&) const;
    bool has(const std::string& name) const { return instructions_.count(name) == 1; }
};

#endif // INSTRUCTIONSPROVIDER_HPP
