#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <Rbo/StatsManager.hpp>

namespace Rbo {

struct InvalidCapacity : std::logic_error {
    InvalidCapacity(const std::string& reason) : std::logic_error { "Inventory has invalid capacity : " + reason } {}
};

struct InvalidQty : std::logic_error {
    InvalidQty(const std::string& item, const std::string& reason) : std::logic_error { "Unable to apply changes for item \"" + item + "\", invalid quantity : " + reason } {}
};

struct UnknownInventory : std::logic_error {
    UnknownInventory(const std::string& name) : std::logic_error { "Unknown inventory \"" + name + '"' } {}
};

struct UnknownItem : std::logic_error {
    UnknownItem(const std::string& item) : std::logic_error { "Unknown item \"" + item + '"' } {}
};

struct PlayerNotDead : std::logic_error {
    PlayerNotDead() : std::logic_error { "This player isn't dead" } {}
};

class Inventory {
private:
    InventorySize capacity_;
    InventoryContent content_;

    void checkExists(const std::string& item) const;
public:
    explicit Inventory(const std::vector<std::string>& items_name, const InventorySize capacity = {});
    Inventory() : Inventory { {}, {} } {}

    bool operator==(const Inventory& rhs) const;

    bool add(const std::string& item, const int qty);
    bool consume(const std::string& item, const int qty);

    int size() const;
    int count(const std::string& item) const;
    bool has(const std::string& item) const { return count(item) != 0; }

    bool limited() const { return capacity_.has_value(); }
    InventorySize capacity() const { return capacity_; }
    bool setCapacity(const InventorySize new_capacity);

    const InventoryContent& content() const { return content_; }
};

class Player {
private:
    using StatsLimits = std::numeric_limits<int>;

    byte id_;
    std::string name_;
    StatsManager stats_;
    Death death_;
    PlayerInventories inventories_;
    ItemsBonus bonuses_;

    enum BonusAction : int {
        None = 0, Enable = 1, Disable = -1
    };

    void readjustStat(const std::string& statName);
    void refreshBonuses(const BonusAction action, const std::string& inv, const std::string& name, const uint qtyModified);

public:
    static constexpr int STAT_MIN { StatsLimits::min() };
    static constexpr int STAT_MAX { StatsLimits::max() };
    static constexpr StatLimits STAT_LIMITS { STAT_MIN, STAT_MAX };

    Player(const byte id, std::string name, const std::vector<std::string>& statsNames, const ItemsList& inventoriesItemsName, const ItemsBonus& bonuses);

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    Player(Player&& other) = default;
    Player& operator=(Player&& rhs) = default;

    bool operator==(const Player&) const = delete;

    bool same(const Player& other) const { return id() == other.id(); }
    byte id() const { return id_; }
    const std::string& name() const { return name_; }

    bool alive() const { return !death_.has_value(); }
    const std::string& death() const;
    void kill(const std::string& reason) { death_ = reason; }

    bool add(const std::string& inventory, const std::string& item, const int qty);
    bool consume(const std::string& inventory, const std::string& item, const int qty);

    Inventory& inventory(const std::string& name);
    const Inventory& inventory(const std::string& name) const;

    StatsManager& stats() { return stats_; }
    const StatsManager& stats() const { return stats_; }

    const PlayerInventories& inventories() const { return inventories_; }
    const ItemsBonus& statsBonus() const { return bonuses_; }
};

template<typename Output>
Output& operator<<(Output& out, const Player& player) {
    out << "[ id=" << std::to_string(player.id()) << "; name=\"" << player.name() << "\"; alive=" << std::boolalpha << player.alive() << std::noboolalpha <<
           " stats=" << player.stats().values() << " inventories=[";
    for (const auto& [name, inventory] : player.inventories())
        out << " \"" << name << "\"=" << inventory << ';';

    out << " ] ]";
    return out; // operator<< retourne osteam& (ref sur classe mère) et non pas Output&
}

template<typename Output>
Output& operator<<(Output& out, const Inventory& inventory) {
    out << "[ capacity=" << (!inventory.limited() ? std::string { "Inf" } : std::to_string(*inventory.capacity())) << "; size=" << inventory.size() << " ]";

    return out; // operator<< retourne osteam& (ref sur classe mère) et non pas Output&
}

} // namespace Rbo

#endif // PLAYER_HPP
