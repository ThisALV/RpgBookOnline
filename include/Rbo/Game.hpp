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
    std::string stat;
    std::string op;
    int value;

    bool test(const StatsManager& stats) const;
};

struct DeathCondition {
    Condition dieIf;
    std::string deathMessage;
};

// Pour l'instant DeathCondition et EndCondition sont traitées de la même manière mais cela pourrait changer.
// Il est donc préférable de faire une structure différente pour ces deux types d'objets.
struct EndCondition {
    Condition stopIf;
    std::string endMessage;
};

struct Game {
    enum struct Error {
        InitialInventories, Bonuses, Effects, Groups, RestGivables,
        RestAvailables, DeathConditions, GameEndConditions
    };

    static constexpr std::string_view getMessage(const Error err_type) {
        switch (err_type) {
        case Error::InitialInventories:
            return "Invalid initial inventories : No more space or unknown items";
        case Error::Bonuses:
            return "Invalid buff : unknown stats or items";
        case Error::Effects:
            return "Invalid effects : unknown stats";
        case Error::Groups:
            return "Invalid enemies group : unknown enemies";
        case Error::RestGivables:
            return "Invalid rest properties : unknown giveable items";
        case Error::RestAvailables:
            return "Invalid rest properties : unknown consumable items or effects";
        case Error::DeathConditions:
            return "Invalid death conditions : unknown stats or condition operators";
        case Error::GameEndConditions:
            return "Invalid game end conditions : unknown stats or condition operators";
        }

        return "This shouldn't happen";
    }

    std::string name;
    StatsDescriptor globalStats;
    StatsDescriptor playerStats;
    InventoriesDescriptor playerInventories;

    ItemsBonus bonuses;
    Effects eventEffects;
    std::unordered_map<std::string, EnemyDescriptor> enemies;
    std::unordered_map<std::string, GroupDescriptor> groups;
    RestProperties rest;
    std::vector<DeathCondition> deathConditions;
    std::vector<EndCondition> gameEndConditions;
    bool voteOnLeaderDeath;
    bool voteLeader;

    std::unordered_map<std::string, Message> messages;

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
};

} // namespace Rbo

#endif // GAME_HPP
