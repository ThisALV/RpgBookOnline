#include <Rbo/Player.hpp>

namespace Rbo {

void Player::refreshBonuses(const BonusAction action, const std::string& inv, const std::string& item, const uint qty) {
    const std::string bonus_key { itemEntry(inv, item) };
    if (statsBonus().count(bonus_key) == 1) {
        const ItemBonus& bonus { statsBonus().at(bonus_key) };

        stats().change(bonus.stat, static_cast<int>(action) * qty * bonus.bonus);
    }
}

Player::Player(const byte id, std::string name, const std::vector<std::string>& stats, const ItemsList& inventories, const ItemsBonus& bonuses) :
    id_ { id },
    name_ { std::move(name) },
    stats_ { stats },
    bonuses_ { bonuses }
{
    std::transform(inventories.cbegin(), inventories.cend(), std::inserter(inventories_, inventories_.begin()), [](const auto& inv) {
        return PlayerInventories::value_type { inv.first, Inventory { inv.second } };
    });
}

const std::string& Player::death() const {
    if (alive())
        throw PlayerNotDead();

    return *death_;
}

bool Player::add(const std::string& inv, const std::string& item, const int qty) {
    const bool ok { inventory(inv).add(item, qty) };
    if (ok)
        refreshBonuses(BonusAction::Enable, inv, item, qty);

    return ok;
}

bool Player::consume(const std::string& inv, const std::string& item, const int qty) {
    const bool ok { inventory(inv).consume(item, qty) };
    if (ok)
        refreshBonuses(BonusAction::Disable, inv, item, qty);

    return ok;
}

Inventory& Player::inventory(const std::string& inv) {
    if (inventories().count(inv) == 0)
        throw UnknownInventory { inv };

    return inventories_.at(inv);
}

const Inventory& Player::inventory(const std::string &inv) const {
    if (inventories().count(inv) == 0)
        throw UnknownInventory { inv };

    return inventories().at(inv);
}

void Inventory::checkExists(const std::string& item) const {
    if (content().count(item) == 0)
        throw UnknownItem { item };
}

namespace {

void checkqQtyForChange(const std::string& item, const int qty) {
    if (qty < 0)
        throw InvalidQty { item, "Can't be negative" };
}

void checkCapacity(const InventorySize capacity) {
    if (capacity && *capacity < 0)
        throw InvalidCapacity { "Can't be negative" };
}

}

Inventory::Inventory(const std::vector<std::string>& items, InventorySize size) : capacity_ { size } {
    checkCapacity(capacity_);

    std::transform(items.cbegin(), items.cend(), std::inserter(content_, content_.end()), [](const std::string& item) -> InventoryContent::value_type {
        return { item, 0 };
    });
}

bool Inventory::operator==(const Inventory& rhs) const {
    return content() == rhs.content() && capacity_ == rhs.capacity_;
}

bool Inventory::add(const std::string& item, const int qty) {
    checkqQtyForChange(item, qty);
    checkExists(item);

    const bool ok { !capacity().has_value() || (size() + qty <= capacity()) };
    if (ok)
        content_.at(item) += qty;

    return ok;
}

bool Inventory::consume(const std::string &item, const int qty) {
    checkqQtyForChange(item, qty);
    checkExists(item);

    const bool ok { count(item) >= static_cast<int>(qty) };
    if (ok)
        content_.at(item) -= qty;

    return ok;
}

int Inventory::count(const std::string& item) const {
    checkExists(item);
    return content().at(item);
}

int accumulate(const int total, const InventoryContent::value_type& qty) {
    return total + qty.second;
}

int Inventory::size() const {
    return std::accumulate(content().cbegin(), content().cend(), int { 0 }, accumulate);
}

bool Inventory::setCapacity(const InventorySize capacity) {
    checkCapacity(capacity);

    const bool ok { !capacity || (*capacity >= size()) };
    if (ok)
        capacity_ = capacity;

    return ok;
}

} // namespace Rbo
