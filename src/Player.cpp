#include <Rbo/Player.hpp>

#include <cassert>
#include <numeric>

namespace Rbo {

void Player::refreshBonuses(const BonusAction action, const std::string& inv,
                            const std::string& item, const uint qty)
{
    const std::string bonus_key { itemEntry(inv, item) };
    if (statsBonus().count(bonus_key) == 1) {
        const ItemBonus& bonus { statsBonus().at(bonus_key) };

        stats().change(bonus.stat, static_cast<int>(action) * qty * bonus.bonus);
    }
}

Player::Player(const byte id, const std::string& name, const std::vector<std::string>& stats,
               const ItemsList& inventories, const ItemsBonuses& bonuses)
    : id_ { id }, name_ { name }, stats_ { stats }, bonuses_ { bonuses }
{
    for (const auto& [name, items] : inventories)
        inventories_.insert({ name, Inventory { items } });
}

bool Player::add(const std::string& inv, const std::string& item, const uint qty) {
    const bool ok { inventory(inv).add(item, qty) };
    if (ok)
        refreshBonuses(BonusAction::Enable, inv, item, qty);

    return ok;
}

bool Player::consume(const std::string& inv, const std::string& item, const uint qty) {
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

Inventory::Inventory(const std::vector<std::string>& items, const InventorySize& size)
    : size_ { size }
{
    std::transform(items.cbegin(), items.cend(), std::inserter(content_, content_.end()),
                   [](const std::string& item) -> InventoryContent::value_type
    {
        return { item, 0 };
    });
}

bool Inventory::operator==(const Inventory& rhs) const {
    return content() == rhs.content() && size_ == rhs.size_;
}

void Inventory::checkExists(const std::string& item) const {
    if (content().count(item) == 0)
        throw UnknownItem { item };
}

bool Inventory::add(const std::string& item, const uint qty) {
    checkExists(item);
    const bool ok { !maxSize().has_value() || (size() + qty <= maxSize()) };
    if (ok)
        content_.at(item) += qty;

    return ok;
}

bool Inventory::consume(const std::string &item, const uint qty) {
    checkExists(item);
    const bool ok { count(item) >= qty };
    if (ok)
        content_.at(item) -= qty;

    return ok;
}

uint Inventory::count(const std::string& item) const {
    checkExists(item);
    return content().at(item);
}

std::size_t accumulate(const std::size_t total, const InventoryContent::value_type& qty) {
    return total + qty.second;
}

uint Inventory::size() const {
    return std::accumulate(content().cbegin(), content().cend(), 0, accumulate);
}

bool Inventory::setMaxSize(const InventorySize capacity) {
    const bool ok { !capacity || (*capacity >= size()) };
    if (ok)
        size_ = capacity;

    return ok;
}

} // namespace Rbo
