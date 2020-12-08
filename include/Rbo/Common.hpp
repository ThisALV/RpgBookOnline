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
#include <limits>
#include <spdlog/fwd.h>

using ushort = std::uint16_t;
using uint = std::uint32_t;
using ulong = std::uint64_t;

namespace Rbo {

class StatsManager;
class Player;
class Inventory;
class Gameplay;
class EventEffect;
class Game;

struct ItemBonus;
struct Stat;
struct StatDescriptor;
struct InventoryDescriptor;
struct PlayerState;
struct EnemyDescriptorBinding;

using byte = std::uint8_t;
using s_byte = std::int8_t;
using word = std::uint16_t;

using RandomEngine = std::mt19937_64;

enum struct ReplyValidity : byte;

using OptionsList = std::map<byte, std::string>;
using Replies = std::map<byte, byte>;
using ReplyController = std::function<void(const byte reply)>;

using Next = std::optional<word>;
using Instruction = std::function<Next(Gameplay& interface)>;
using Scene = std::vector<Instruction>;
using SceneBuilder = std::function<Scene(const Game& currentGame, const word scene)>;

using Players = std::map<byte, Player*>;
using ConstPlayers = std::map<byte, const Player*>;

using InventorySize = std::optional<uint>;
using InventoryContent = std::unordered_map<std::string, uint>;
using InventoriesSize = std::unordered_map<std::string, InventorySize>;
using InventoriesContent = std::unordered_map<std::string, InventoryContent>;
using PlayerInventories = std::unordered_map<std::string, Inventory>;
using ItemsList = std::unordered_map<std::string, std::vector<std::string>>;
using ItemsBonus = std::unordered_map<std::string, ItemBonus>;

using StatsValue = std::unordered_map<std::string, int>;
using StatsValueLimits = std::numeric_limits<int>;
using Stats = std::unordered_map<std::string, Stat>;

using PlayersState = std::map<byte, PlayerState>;
using InventoryUpdate = std::unordered_map<std::string, int>;
using InventoriesUpdate = std::unordered_map<std::string, InventoryUpdate>;

using StatsDescriptor = std::unordered_map<std::string, StatDescriptor>;
using InventoriesDescriptor = std::unordered_map<std::string, InventoryDescriptor>;

using GroupDescriptor = std::map<byte, EnemyDescriptorBinding, std::greater<byte>>;

struct DiceFormula {
    uint dices;
    int bonus;

    int operator()() const;
    int min() const { return dices + bonus; }
    int max() const { return dices * 6 + bonus; };

private:
    static RandomEngine rd_;
    static std::uniform_int_distribution<uint> rd_distributor_;
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

    InvalidReply(const ReplyValidity errorType);
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
    Stats stats;
    InventoriesUpdate items;
    InventoriesSize capacities;
};

struct PlayerState {
    Stats stats;
    InventoriesContent inventories;
    InventoriesSize capacities;
};

struct PlayerCache {
    bool initialized;
    PlayerState playerState;
};

struct StatDescriptor {
    DiceFormula initialValue;
    StatLimits limits;
    bool capped;
    bool hidden;
    bool main;
};

struct InventoryDescriptor {
    std::optional<DiceFormula> limit;
    std::vector<std::string> items;
    InventoryContent initialStuff;
};

ulong now();
std::string itemEntry(const std::string& inventory, const std::string& item);
std::pair<std::string, std::string> splitItemEntry(const std::string& itemEntry);
bool contains(const std::vector<std::string>& strs, const std::string& element);
byte vote(const Replies& requestResults);

spdlog::logger& rboLogger(const std::string& name);

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

const byte ALL_PLAYERS { 0 };
const word INTRO { 0 };

} // namespace Rbo

namespace std {

template<typename Output>
Output& operator<<(Output& out, const std::vector<Rbo::byte>& ids) {
    out << '[';
    for (const Rbo::byte id : ids)
        out << ' ' << std::to_string(id) << ';';

    out << " ]";
    return out; // (ostringstream& << string) ne retourne pas une ostringstream&
}

}

#endif // COMMON_HPP

