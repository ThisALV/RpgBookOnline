#include <Rbo/Tests/TestsCommon.hpp>

#include <iostream>
#include "Rbo/Player.hpp"

namespace Rbo {

std::ostream& operator<<(std::ostream& out, const PlayerInventories& inventories) {
    out << "PlayerInventories :";

    for (const auto& [name, inventory] : inventories) {
        out << " \"" << name << "\"=[ " << (inventory.limited() ? std::to_string(inventory.size()) : std::string { "Inf" });

        for (const auto& [item, qty] : inventory.content())
            out << " \"" << item << "\"=" << std::to_string(qty);

        out << " ]";
    }

    return out;
}

std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& strs) {
    out << "[Size : " << strs.size() << "]";

    for (const std::string& str : strs)
        out << " \"" << str << "\";";

    return out;
}

} // namespace Rbo
