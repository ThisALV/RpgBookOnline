#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "Common.hpp"

#include "StatsManager.hpp"

namespace Rbo {

struct UnknownInventory : std::logic_error {
    UnknownInventory(const std::string& name)
        : std::logic_error { "Inventoraire \"" + name + "\" inconnu" } {}
};

struct UnknownItem : std::logic_error {
    UnknownItem(const std::string& item)
        : std::logic_error { "Item \"" + item + "\" inconnu" } {}
};

class Inventory {
public:
    explicit Inventory(const std::vector<std::string>&, const InventorySize& = {});
    Inventory() : Inventory { {}, {} } {}

    bool operator==(const Inventory&) const;

    bool add(const std::string&, const uint);
    bool consume(const std::string&, const uint);

    uint size() const;
    uint count(const std::string&) const;
    bool has(const std::string& item) const { return count(item) != 0; }

    bool limited() const { return size_.has_value(); }
    InventorySize maxSize() const { return size_; }
    bool setMaxSize(const InventorySize&);

    const InventoryContent& content() const { return content_; }

private:
    InventorySize size_;
    InventoryContent content_;

    void checkExists(const std::string&) const;
};

class Player {
private:
    using StatsLimits = std::numeric_limits<int>;

    byte id_;
    std::string name_;
    StatsManager stats_;
    PlayerInventories inventories_;
    ItemsBonuses bonuses_;

    enum BonusAction : int {
        None = 0, Enable = 1, Disable = -1
    };

    void readjustStat(const std::string&);
    void refreshBonuses(const BonusAction, const std::string&, const std::string&, const uint);

public:
    friend void testRefreshBonuses(Player&, const int, const std::string&, const std::string&,
                                   const uint);

    inline static const int STAT_MIN { StatsLimits::min() };
    inline static const int STAT_MAX { StatsLimits::max() };
    inline static const StatLimits STAT_LIMITS { STAT_MIN, STAT_MAX };

    Player(const byte, const std::string&, const std::vector<std::string>&, const ItemsList&,
           const ItemsBonuses&);

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    Player(Player&&) = default;
    Player& operator=(Player&&) = default;

    bool operator==(const Player&) const = delete;

    bool same(const Player& p) const { return id() == p.id(); }
    byte id() const { return id_; }
    const std::string& name() const { return name_; }

    bool add(const std::string&, const std::string&, const uint);
    bool consume(const std::string&, const std::string&, const uint);

    Inventory& inventory(const std::string&);
    const Inventory& inventory(const std::string&) const;

    StatsManager& stats() { return stats_; }
    const StatsManager& stats() const { return stats_; }

    const PlayerInventories& inventories() const { return inventories_; }
    const ItemsBonuses statsBonus() const { return bonuses_; }
};

} // namespace Rbo

template<typename Output> Output& operator<<(Output& out, const Rbo::Player& player) {
    out << "[ id=" << std::to_string(player.id()) << "; name=\"" << player.name()
        << "\"; stats=" << player.stats().values() << " inventories=[";
    for (const auto& [name, inventory] : player.inventories())
        out << " \"" << name << "\"=" << inventory << ';';

    out << " ] ]";
    return out;
}

template<typename Output>
Output& operator<<(Output& out, const Rbo::Inventory& inventory) {
    out << "[ maxSize=" << (inventory.limited() ? std::string { "Inf" }
                                                : std::to_string(*inventory.maxSize()))
        << "; size=" << inventory.size() << " ]";

    return out;
}

#endif // PLAYER_HPP
