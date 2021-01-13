#ifndef COMMON_HPP
#define COMMON_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <spdlog/fwd.h>

using ushort = std::uint16_t;
using uint = std::uint32_t;
using ulong = std::uint64_t;

namespace Rbo {

class StatsManager;
class Enemy;
class Player;
class Inventory;
class Gameplay;

struct EventEffect;
struct Game;
struct ItemBonus;
struct Stat;
struct StatDescriptor;
struct InventoryDescriptor;
struct PlayerState;
struct EnemyDescriptorBinding;
struct RollResult;

using byte = std::uint8_t;
using word = std::uint16_t;

using RandomEngine = std::mt19937_64;

enum struct ReplyValidity : byte;

using OptionsList = std::vector<std::string>;
using Replies = std::unordered_map<byte, byte>;
using DiceRollResults = std::map<byte, RollResult>; // Triés pour les tests unitaires sur la fabrication de paquets
using ReplyController = std::function<void(const byte reply)>;

using Next = std::optional<word>;
using Instruction = std::function<Next(Gameplay& interface)>;
using Scene = std::vector<Instruction>;

using Players = std::map<byte, Player*>;
using ConstPlayers = std::map<byte, const Player*>;

using InventorySize = std::optional<int>;
using InventoryContent = std::unordered_map<std::string, int>;
using InventoriesSize = std::unordered_map<std::string, InventorySize>;
using InventoriesContent = std::unordered_map<std::string, InventoryContent>;
using PlayerInventories = std::unordered_map<std::string, Inventory>;
using ItemsList = std::unordered_map<std::string, std::vector<std::string>>;
using ItemsBonus = std::unordered_map<std::string, ItemBonus>;

using StatsValue = std::unordered_map<std::string, int>;
using StatsValueLimits = std::numeric_limits<int>;
using Stats = std::unordered_map<std::string, Stat>;

using Death = std::optional<std::string>;

using PlayersState = std::unordered_map<byte, PlayerState>;
using InventoryUpdate = std::unordered_map<std::string, int>;
using InventoriesUpdate = std::unordered_map<std::string, InventoryUpdate>;

using StatsDescriptor = std::unordered_map<std::string, StatDescriptor>;
using InventoriesDescriptor = std::unordered_map<std::string, InventoryDescriptor>;

using GroupDescriptor = std::vector<EnemyDescriptorBinding>;
using EnemiesQueue = std::vector<Enemy>;

using Message = std::optional<std::string>;
using Messages = std::unordered_map<std::string, Message>;

template<typename Outputable> struct OutputableWrapper;
using StatsValueWrapper = OutputableWrapper<StatsValue>;
using RepliesWrapper = OutputableWrapper<Replies>;
template<typename T> using VectorWrapper = OutputableWrapper<std::vector<T>>;
using ByteVecWrapper = VectorWrapper<byte>;

struct RollResult  {
    std::vector<byte> dices;
    int bonus;

    int total() const;
};

struct DicesRoll {
    byte dices;
    int bonus;

    RollResult operator()() const;
    int min() const { return dices + bonus; }
    int max() const { return dices * 6 + bonus; };
};

struct ItemBonus {
    std::string stat;
    int bonus;

    bool operator==(const ItemBonus& rhs) const;
};

struct EnemyDescriptor {
    int hp;
    int skill;
};

struct EnemyDescriptorBinding {
    std::string ctxName;
    std::string genericName;
};

struct InvalidReply : std::logic_error {
    ReplyValidity type;

    explicit InvalidReply(const ReplyValidity errorType);
};

struct PlayerCheckingResult {
    bool alive;
    bool leaderSwitch;
    bool sessionEnd;
};

struct StatLimits {
    int min { StatsValueLimits::min() };
    int max { StatsValueLimits::max() };

    bool operator==(const StatLimits&) const;
};

struct Stat {
    int value { 0 };
    StatLimits limits;
    bool hidden { false };
    bool main { false };

    bool operator==(const Stat& rhs) const;
    bool operator!=(const Stat& rhs) const { return !(*this == rhs); }
};

struct PlayerUpdate {
    Death death;
    Stats stats;
    InventoriesUpdate items;
    InventoriesSize capacities;
};

struct PlayerState {
    Death death;
    Stats stats;
    InventoriesContent inventories;
    InventoriesSize capacities;
};

struct PlayerCache {
    bool initialized;
    PlayerState playerState;
};

struct StatDescriptor {
    DicesRoll initialValue;
    StatLimits limits;
    bool capped;
    bool hidden;
    bool main;
};

struct InventoryDescriptor {
    std::optional<DicesRoll> limit;
    std::vector<std::string> items;
    InventoryContent initialStuff;
};

template<typename Output>
Output& operator<<(Output& out, const std::vector<byte>& ids) {
    out << '[';
    for (const byte id : ids)
        out << ' ' << std::to_string(id) << ';';

    out << " ]";
    return out; // operator<< retourne osteam& (ref sur classe mère) et non pas Output&
}

template<typename Output>
Output& operator<<(Output& out, const StatsValue& stats) {
    out << '[';
    for (const auto&[name, value] : stats)
        out << " \"" << name << "\"=" << value << ';';

    out << " ]";
    return out; // operator<< retourne osteam& (ref sur classe mère) et non pas Output&
}

template<typename Output>
Output& operator<<(Output& out, const Replies& replies) {
    out << '[';
    for (const auto [id, reply] : replies)
        out << ' ' << std::to_string(id) << "->" << std::to_string(reply);

    out << " ]";
    return out; // operator<< retourne osteam& (ref sur classe mère) et non pas Output&
}

// Pour pouvoir utiliser operator<< avec des using dans Rbo sur des types de la std en utilisant l'ADL
template<typename Outputable>
struct OutputableWrapper {
    const Outputable& value;

    // Pour utiliser le wrapper dans les tests de Boost
    bool operator==(const OutputableWrapper<Outputable> rhs) const { return value == rhs.value; }
};

template<typename Output, typename Outputable>
Output& operator<<(Output& out, const OutputableWrapper<Outputable>& wrapper) {
    Rbo::operator<<(out, wrapper.value); // Préciser le ns Rbo permet de rechercher les operator<< définis par la lib rbo
    return out; // operator<< retourne osteam& (ref sur classe mère) et non pas Output&
}

ulong now();
std::string itemEntry(const std::string& inventory, const std::string& item);
std::pair<std::string, std::string> splitItemEntry(const std::string& itemEntry);
std::optional<byte> vote(const Replies& requestResults);

spdlog::logger& rboLogger(std::string name);

template<typename NumType>
std::vector<byte> decompose(const NumType value) {
    const NumType byte_mask { 0xff };
    const std::size_t length { sizeof(NumType) };

    std::vector<byte> bytes;
    bytes.resize(length);

    for (std::size_t i { 0 }; i < length; i++)
        bytes[length - i - 1] = (value >> (8 * i)) & byte_mask;

    return bytes;
}

constexpr char ITEM_ENTRY_SEP { '/' };

constexpr byte ALL_PLAYERS { std::numeric_limits<byte>::max() };
constexpr byte ACTIVE_PLAYERS { std::numeric_limits<byte>::max() - 1 };

constexpr byte YES { 0 };
constexpr byte NO { 1 };

constexpr word INTRO { 0 };

} // namespace Rbo

#endif // COMMON_HPP

