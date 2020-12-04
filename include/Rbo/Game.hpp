#ifndef GAME_HPP
#define GAME_HPP

#include <Rbo/Common.hpp>

namespace Rbo {

struct EventEffect {
    enum struct ItemsChanges {
        Ok, InvFull, ItemEmpty
    };

    std::unordered_map<std::string, int> statsChanges;
    std::unordered_map<std::string, int> itemsChanges;

    bool operator==(const EventEffect& rhs) const;

    void apply(Player& target) const;
    ItemsChanges simulateItemsChanges(const Player& possibleTarget) const;
};

using Effects = std::unordered_map<std::string, EventEffect>;

struct RestProperties {
    std::vector<std::string> givables;
    std::unordered_map<std::string, std::string> availables;
};

struct Condition {
    using Operator = std::function<bool(const int statValue, const int conditionThreshold)>;

    static const std::unordered_map<std::string, Operator> operators;

    std::string stat;
    std::string op;
    int value;

    bool test(const StatsManager& stats) const;
};

struct Game {
    enum struct Error {
        InitialInventories, Bonuses, Effects, Groups, RestGivables,
        RestAvailables, DeathConditions, GameEndConditions
    };

    static std::string getMessage(const Error errType);

    std::string name;
    StatsDescriptors globalStats;
    StatsDescriptors playerStats;
    InventoriesDescriptors playerInventories;

    ItemsBonuses bonuses;
    Effects eventEffects;
    std::unordered_map<std::string, EnemyDescriptor> enemies;
    std::unordered_map<std::string, GroupDescriptor> groups;
    RestProperties rest;
    std::vector<Condition> deathConditions;
    std::vector<Condition> gameEndConditions;
    bool voteOnLeaderDeath;
    bool voteLeader;

    const EventEffect& effect(const std::string& name) const;
    const EnemyDescriptor& enemy(const std::string& name) const;
    const GroupDescriptor& group(const std::string& name) const;

    std::vector<std::string> player() const;
    std::vector<std::string> global() const;
    ItemsList itemsList() const;

    std::vector<Error> validity() const;

    bool hasGlobal(const std::string& stat) const { return globalStats.count(stat) == 1; }
    bool hasStat(const std::string& stat) const { return playerStats.count(stat) == 1; }
    bool hasEffect(const std::string& name) const { return eventEffects.count(name) == 1; }
    bool hasEnemy(const std::string& generic_name) const { return enemies.count(generic_name) == 1; }
    bool hasItem(const std::string& itemEntry) const;

private:
    static std::vector<std::string> getNames(const StatsDescriptors& stats);
};

} // namespace Rbo

#endif // GAME_HPP
