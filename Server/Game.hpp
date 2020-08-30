#ifndef GAME_HPP
#define GAME_HPP

#include "Common.hpp"

namespace Rbo {

struct EventEffect {
    enum struct ItemsChanges {
        Ok, InvFull, ItemEmpty
    };

    std::unordered_map<std::string, int> statsChanges;
    std::unordered_map<std::string, int> itemsChanges;

    bool operator==(const EventEffect&) const;

    void apply(Player&) const;
    ItemsChanges simulateItemsChanges(const Player&) const;
};

using Effects = std::unordered_map<std::string, EventEffect>;

struct RestProperties {
    std::vector<std::string> givables;
    std::unordered_map<std::string, std::string> availables;
};

struct Condition {
    using Operator = std::function<bool(const int, const int)>;

    static const std::unordered_map<std::string, Operator> operators;

    std::string stat;
    std::string op;
    int value;

    bool test(const StatsManager&) const;
};

struct Game {
    enum struct Validity {
        Ok, InitialInventories, Bonuses, Effects, RestGivables, RestAvailables, DeathConditions,
        GameEndConditions
    };

    static std::string getMessage(const Validity);

    std::string name;
    StatsDescriptors globalStats;
    StatsDescriptors playerStats;
    InventoriesDescriptors playerInventories;

    ItemsBonuses bonuses;
    Effects eventEffects;
    RestProperties rest;
    std::vector<Condition> deathConditions;
    std::vector<Condition> gameEndConditions;
    bool voteOnLeaderDeath;
    bool voteLeader;

    const EventEffect& effect(const std::string& name) const { return eventEffects.at(name); }
    std::vector<std::string> player() const;
    std::vector<std::string> global() const;
    ItemsList itemsList() const;

    Validity validity() const;

    bool hasGlobal(const std::string& stat) const { return globalStats.count(stat) == 1; }
    bool hasStat(const std::string& stat) const { return playerStats.count(stat) == 1; }
    bool hasEffect(const std::string& name) const { return eventEffects.count(name) == 1; }
    bool hasItem(const std::string&) const;

private:
    static std::vector<std::string> getNames(const StatsDescriptors&);
};

} // namespace Rbo

#endif // GAME_HPP
