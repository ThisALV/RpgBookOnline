#ifndef COMMON_HPP
#define COMMON_HPP

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <functional>
#include <optional>
#include "spdlog/fwd.h"

namespace Rbo {

using byte = std::uint8_t;
using s_byte = std::int8_t;
using word = std::uint16_t;

struct DiceFormula {
    uint dices;
    int bonus;

    int operator()() const;
    int min() const { return dices + bonus; }
    int max() const { return dices * 6 + bonus; };
};

struct ItemBonus {
    std::string stat;
    int bonus;

    bool operator==(const ItemBonus&) const;
};

using ItemsBonuses = std::unordered_map<std::string, ItemBonus>;

struct EnemyInitializer {
    std::string name;
    int endurance;
    int skill;
};

enum struct ReplyValidity : byte;

struct InvalidReply : std::logic_error {
    ReplyValidity type;

    InvalidReply(const ReplyValidity);
};

struct PlayerCheckingResult {
    bool alive;
    bool leaderSwitch;
    bool sessionEnd;
};

using OptionsList = std::map<byte, std::string>;
using Replies = std::map<byte, byte>;
using ReplyController = std::function<void(const byte)>;

std::string itemEntry(const std::string&, const std::string&);
std::pair<std::string, std::string> splitItemEntry(const std::string&);
bool contains(const std::vector<std::string>&, const std::string&);
byte vote(const Replies&);

spdlog::logger& rboLogger(const std::string&);

template<typename NumType> std::vector<byte> decompose(const NumType value) {
    const NumType byte_mask { 0xff };
    const std::size_t length { sizeof(NumType) };

    std::vector<byte> bytes;
    bytes.resize(length);

    for (std::size_t i { 0 }; i < length; i++)
        bytes[length - i - 1] = (value >> (8 * i)) & byte_mask;

    return bytes;
}

class Player;

class Gameplay;
class Game;

const byte ALL_PLAYERS { 0 };
const word INTRO { 0 };

using Next = std::optional<word>;
using Instruction = std::function<Next(Gameplay&)>;
using Scene = std::vector<Instruction>;
using Options = std::map<std::string, word>;
using SceneBuilder = std::function<Scene(const Game&, const word)>;

using Players = std::map<byte, Player*>;
using ConstPlayers = std::map<byte, const Player*>;

class Inventory;

using InventorySize = std::optional<uint>;
using InventoryContent = std::unordered_map<std::string, uint>;
using PlayerInventories = std::unordered_map<std::string, Inventory>;
using ItemsList = std::unordered_map<std::string, std::vector<std::string>>;

using StatsValues = std::unordered_map<std::string, int>;
using StatValueLimits = std::numeric_limits<int>;

struct StatLimits {
    int min { StatValueLimits::min() };
    int max { StatValueLimits::max() };

    bool operator==(const StatLimits&) const;
};

struct Stat {
    int value { 0 };
    StatLimits limits;
    bool hidden { false };

    bool operator==(const Stat&) const;
    bool operator!=(const Stat& rhs) const { return !(*this == rhs); }
};

using Stats = std::unordered_map<std::string, Stat>;

struct PlayerStateChanges {
    std::unordered_map<std::string, Stat> statsChanges;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> itemsChanges;
    std::unordered_map<std::string, std::size_t> capacitiesChanges;
};

struct PlayerState {
    Stats stats;
    std::unordered_map<std::string, InventoryContent> inventories;
    std::unordered_map<std::string, InventorySize> inventoriesMaxCapacity;
};

using PlayersStates = std::map<byte, PlayerState>;

class StatsManager;

struct StatInitilizer {
    DiceFormula initialValue;
    StatLimits limits;
    bool capped;
    bool hidden;
};

struct InventoryInitializer {
    std::optional<DiceFormula> limit;
    std::vector<std::string> items;
    InventoryContent initialStuff;
};

using StatsInitializers = std::unordered_map<std::string, StatInitilizer>;
using InventoriesInitializers = std::unordered_map<std::string, InventoryInitializer>;

using EnemyInitializers = std::map<byte, EnemyInitializer>;

} // namespace Rbo

template<typename Output> Output& operator<<(Output& out, const std::vector<Rbo::byte>& ids) {
    out << '[';
    for (const Rbo::byte id : ids)
        out << ' ' << std::to_string(id) << ';';

    out << " ]";
    return out; // (ostringstream& << string) ne retourne pas une ostringstream&
}

#endif // COMMON_HPP
