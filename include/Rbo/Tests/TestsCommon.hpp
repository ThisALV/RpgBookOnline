#ifndef TEST_COMMON_HPP
#define TEST_COMMON_HPP

#include <Rbo/Common.hpp>

using namespace Rbo;

namespace Rbo {

template<typename Outputable> struct OutputWrapper {
    const Outputable& value;

    bool operator==(const OutputWrapper<Outputable> rhs) const { return value == rhs.value; }
};

using StatsWrapper = OutputWrapper<StatsValues>;
using InventoriesWrapper = OutputWrapper<PlayerInventories>;
using StrVecWrapper = OutputWrapper<std::vector<std::string>>;

template<typename Outputable>
std::ostream& operator<<(std::ostream& out, OutputWrapper<Outputable> wrapper) {
    return out << wrapper.value;
}

std::ostream& operator<<(std::ostream& out, const PlayerInventories& inventories);
std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& strs);

} // namespace Rbo


#endif // TEST_COMMON_HPP
