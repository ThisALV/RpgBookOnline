#ifndef TEST_COMMON_HPP
#define TEST_COMMON_HPP

#include <Rbo/Common.hpp>

#include <Rbo/Player.hpp>

template<typename Outputable> struct OutputWrapper {
    const Outputable& value;

    bool operator==(const OutputWrapper<Outputable> rhs) const { return value == rhs.value; }
};

using InventoriesWrapper = OutputWrapper<Rbo::PlayerInventories>;
using StatsWrapper = OutputWrapper<Rbo::StatsValues>;
using StrVecWrapper = OutputWrapper<std::vector<std::string>>;

template<typename Outputable>
std::ostream& operator<<(std::ostream& out, OutputWrapper<Outputable> wrapper) {
    return out << wrapper.value;
}

std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& strs) {
    out << "[Size : " << strs.size() << "]";

    for (const std::string& str : strs)
        out << " \"" << str << "\";";

    return out;
}

namespace Rbo {

std::ostream& operator<<(std::ostream& out, const Rbo::PlayerInventories& inventories) {
    out << "PlayerInventories :";

    for (const auto& [name, inventory] : inventories) {
        out << " \"" << name << "\"=[ " << (inventory.limited() ? std::to_string(inventory.size()) : std::string { "Inf" });

        for (const auto& [item, qty] : inventory.content())
            out << " \"" << item << "\"=" << std::to_string(qty);

        out << " ]";
    }

    return out;
}

} // namespace Rbo

#endif // TEST_COMMON_HPP
