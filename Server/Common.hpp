#ifndef COMMON_HPP
#define COMMON_HPP

#include <stdexcept>
#include <array>
#include <iterator>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <functional>
#include <optional>
#include <random>
#include "spdlog/fwd.h"

using ushort = std::uint16_t;
using uint = std::uint32_t;
using ulong = std::uint64_t;

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

private:
    static std::mt19937_64 rd_;
    static std::uniform_int_distribution<uint> rd_distributor_;
};

struct ItemBonus {
    std::string stat;
    int bonus;

    bool operator==(const ItemBonus&) const;
};

using ItemsBonuses = std::unordered_map<std::string, ItemBonus>;

struct EnemyDescriptor {
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

ulong now();
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

struct InventoryDescriptor {
    std::optional<DiceFormula> limit;
    std::vector<std::string> items;
    InventoryContent initialStuff;
};

using StatsDescriptors = std::unordered_map<std::string, StatInitilizer>;
using InventoriesDescriptors = std::unordered_map<std::string, InventoryDescriptor>;

using GroupDescriptor = std::map<byte, EnemyDescriptor>;

} // namespace Rbo

template<typename Output> Output& operator<<(Output& out, const std::vector<Rbo::byte>& ids) {
    out << '[';
    for (const Rbo::byte id : ids)
        out << ' ' << std::to_string(id) << ';';

    out << " ]";
    return out; // (ostringstream& << string) ne retourne pas une ostringstream&
}

#endif // COMMON_HPP
