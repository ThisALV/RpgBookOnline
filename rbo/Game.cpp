#include <Rbo/Game.hpp>

#include <Rbo/Player.hpp>

namespace Rbo {

namespace  {

using Operator = std::function<bool(const int statValue, const int conditionThreshold)>;

const std::unordered_map<std::string, Operator> operators {
    { "==", std::equal_to {} },
    { "!=", std::not_equal_to {} },
    { "<=", std::less_equal {} },
    { "=<", std::less_equal {} },
    { ">=", std::greater_equal {} },
    { "=>", std::greater_equal {} },
    { "<", std::less {} },
    { ">", std::greater {} }
};

}

bool EventEffect::operator==(const EventEffect& rhs) const {
    return statsChanges == rhs.statsChanges && itemsChanges == rhs.itemsChanges;
}

void EventEffect::apply(Player& target) const {
    for (const auto& [name, value] : statsChanges)
        target.stats().change(name, value);

    using ItemEntry = std::pair<std::string, int>;

    std::vector<ItemEntry> ordered_items_changes { itemsChanges.size() };
    std::copy(itemsChanges.cbegin(), itemsChanges.cend(), ordered_items_changes.begin());
    std::partition(ordered_items_changes.begin(), ordered_items_changes.end(), [](const auto& c) -> bool {
        return c.second < 0;
    });

    for (const auto& [item, qty] : ordered_items_changes) {
        const auto& [inventory, name] { splitItemEntry(item) };

        if (qty > 0)
            target.add(inventory, name, qty);
        else
            target.consume(inventory, name, std::abs(qty));
    }
}

EventEffect::ItemsChanges EventEffect::simulateItemsChanges(const Player& target) const {
    std::unordered_map<std::string, InventorySize::value_type> supposed_sizes;

    for (const auto& [name, inv] : target.inventories()) {
        if (inv.limited())
            supposed_sizes.insert({ name, inv.size() });
    }

    for (const auto& [item, qty] : itemsChanges) {
        const auto& [inv, name] { splitItemEntry(item) };
        const Inventory& target_inv { target.inventory(inv) };

        if (qty < 0 && static_cast<int>(target_inv.count(name)) < std::abs(qty))
            return ItemsChanges::ItemEmpty;

        if (target_inv.limited())
            supposed_sizes.at(inv) += qty;
    }

    const bool full = std::any_of(supposed_sizes.cbegin(), supposed_sizes.cend(), [&target](const auto& size) {
        return size.second > target.inventory(size.first).capacity();
    });

    return full ? ItemsChanges::InvFull : ItemsChanges::Ok;
}

bool Condition::test(const StatsManager& stats) const {
    assert(operators.count(op) == 1);
    return operators.at(op)(stats.get(stat), value);
}

const EventEffect& Game::effect(const std::string& name) const {
    if (!hasEffect(name))
        throw std::out_of_range { "Effet \"" + name + "\" inconnu" };

    return eventEffects.at(name);
}

const EnemyDescriptor& Game::enemy(const std::string& generic_name) const {
    if (!hasEnemy(generic_name))
        throw std::out_of_range { "Descipteur d'ennemi \"" + name + "\" inconnu" };

    return enemies.at(generic_name);
}

const GroupDescriptor& Game::group(const std::string& name) const {
    if (groups.count(name) == 0)
        throw std::out_of_range { "Descipteur de groupe d'ennemis \"" + name + "\" inconnu" };

    return groups.at(name);
}

std::vector<Game::Error> Game::validity() const {
    std::vector<Error> errors;

    const bool valid_init_invs = std::all_of(playerInventories.cbegin(), playerInventories.cend(), [](const auto& i) {
        const InventoryDescriptor& inv { i.second };

        int init_size { 0 };
        return std::all_of(inv.initialStuff.cbegin(), inv.initialStuff.cend(), [&init_size, &inv](const auto& item) {
            if (inv.limit && (init_size += item.second) > inv.limit->min())
                return false;

            const auto cb { inv.items.cbegin() };
            const auto ce { inv.items.cend() };

            return std::find(cb, ce, item.first) != ce;
        });
    });

    if (!valid_init_invs)
        errors.push_back(Error::InitialInventories);

    const bool valid_bonuses = std::all_of(bonuses.cbegin(), bonuses.cend(), [this](const auto& b) {
        return hasItem(b.first) && hasStat(b.second.stat);
    });

    if (!valid_bonuses)
        errors.push_back(Error::Bonuses);

    const bool valid_effects = std::all_of(eventEffects.cbegin(), eventEffects.cend(), [this](const auto& e) {
        const EventEffect& effect { e.second };

        const bool valid_stats = std::all_of(effect.statsChanges.cbegin(), effect.statsChanges.cend(), [this](const auto& s) {
            return hasStat(s.first) || hasGlobal(s.first);
        });

        if (!valid_stats)
            return false;

        const bool valid_items = std::all_of(effect.itemsChanges.cbegin(), effect.itemsChanges.cend(), [this](const auto& i) {
            return hasItem(i.first);
        });

        if (!valid_items)
            return false;

        return true;
    });

    if (!valid_effects)
        errors.push_back(Error::Effects);

    const bool valid_groups = std::all_of(groups.cbegin(), groups.cend(), [this](const auto& g) {
        return std::all_of(g.second.cbegin(), g.second.cend(), [this](const auto& e) {
            return hasEnemy(e.genericName);
        });
    });

    if (!valid_groups)
        errors.push_back(Error::Groups);

    const bool valid_rest_givables = std::all_of(rest.givables.cbegin(), rest.givables.cend(), [this](const auto& i) {
        return hasItem(i);
    });

    if (!valid_rest_givables)
        errors.push_back(Error::RestGivables);

    const bool valid_rest_availables = std::all_of(rest.availables.cbegin(), rest.availables.cend(), [this](const auto& a) {
        return hasItem(a.first) && hasEffect(a.second);
    });

    if (!valid_rest_availables)
        errors.push_back(Error::RestAvailables);

    const bool valid_death_conditions = std::all_of(deathConditions.cbegin(), deathConditions.cend(), [this](const auto& c) {
        const Condition& condition { c.dieIf };

        return operators.count(condition.op) == 1 && hasStat(condition.stat);
    });

    if (!valid_death_conditions)
        errors.push_back(Error::DeathConditions);

    const bool valid_end_conditions = std::all_of(gameEndConditions.cbegin(), gameEndConditions.cend(), [this](const auto& c) {
        const Condition& condition { c.stopIf };

        return operators.count(condition.op) == 1 && hasGlobal(condition.stat);
    });

    if (!valid_end_conditions)
        errors.push_back(Error::GameEndConditions);

    return errors;
}

bool Game::hasItem(const std::string& entry) const {
    const auto [inv, item] { splitItemEntry(entry) };
    if (playerInventories.count(inv) == 0)
        return false;

    const auto items_begin { playerInventories.at(inv).items.cbegin() };
    const auto items_end { playerInventories.at(inv).items.cend() };

    return std::find(items_begin, items_end, item) != items_end;
}

namespace {

std::vector<std::string> getStatsName(const StatsDescriptor& stats) {
    std::vector<std::string> names { stats.size() };
    std::transform(stats.cbegin(), stats.cend(), names.begin(), [](const auto& stat) -> std::string {
        return stat.first;
    });

    return names;
}

}

std::vector<std::string> Game::global() const { return getStatsName(globalStats); }
std::vector<std::string> Game::player() const { return getStatsName(playerStats); }

ItemsList Game::itemsList() const {
    ItemsList items;
    std::transform(playerInventories.cbegin(), playerInventories.cend(), std::inserter(items, items.begin()), [](const auto& inv) -> ItemsList::value_type {
        return { inv.first, inv.second.items };
    });

    return items;
}

} // namespace Rbo
