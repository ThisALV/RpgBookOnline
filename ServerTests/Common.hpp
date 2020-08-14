#ifndef TEST_COMMON_HPP
#define TEST_COMMON_HPP

#include "Rbo/Common.hpp"

std::ostream& operator<<(std::ostream&, const StatsValues&);
std::ostream& operator<<(std::ostream&, const PlayerInventories&);

template<typename Outputable> struct OutputWrapper {
    const Outputable& value;

    bool operator==(const OutputWrapper<Outputable> rhs) const { return value == rhs.value; }
};

using StatsWrapper = OutputWrapper<StatsValues>;
using InventoriesWrapper = OutputWrapper<PlayerInventories>;

template<typename Outputable>
std::ostream& operator<<(std::ostream& out, OutputWrapper<Outputable> wrapper) {
    return out << wrapper.value;
}


#endif // TEST_COMMON_HPP
