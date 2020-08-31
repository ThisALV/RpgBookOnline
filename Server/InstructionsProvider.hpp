#ifndef INSTRUCTIONSPROVIDER_HPP
#define INSTRUCTIONSPROVIDER_HPP

#include "Common.hpp"

#include "TablesLock.hpp"

namespace Rbo::Server {

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

    static std::vector<byte> getIDs(const Replies&);
    static std::tuple<byte, byte> reply(const Replies&);
    static void assertArgs(const bool);
    static bool toBoolean(const std::string&);
    static void applyToGlobal(Gameplay&, const EventEffect&);
    static bool isInstruction(const sol::object&, const sol::object&);

    struct LuaInstruction {
        LuaFunc func;
        std::vector<std::string> args;

        Next operator()(Gameplay&) const;
    };

    Instructions instructions_;
    sol::state& ctx_;
    spdlog::logger& logger_;
    TablesLock resources_lock_;

    void initBuiltins();

public:
    InstructionsProvider(sol::state&, spdlog::logger&);

    InstructionsProvider(const InstructionsProvider&) = delete;
    InstructionsProvider& operator=(const InstructionsProvider&) = delete;

    bool operator==(const InstructionsProvider&) const = delete;

    void load();
    Instruction get(const std::string&, const std::vector<std::string>&) const;
    bool has(const std::string& name) const { return instructions_.count(name) == 1; }
};

} // namespace Rbo::Server

#endif // INSTRUCTIONSPROVIDER_HPP
