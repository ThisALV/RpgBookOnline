#define BOOST_TEST_MODULE SessionDataFactory

#include "boost/test/unit_test.hpp"
#include "nlohmann/json.hpp"
#include "Rbo/SessionDataFactory.hpp"

using namespace Rbo; // Common.hpp nécessite d'include Player.hpp

using json = nlohmann::json;

namespace Rbo {

std::ostream& operator<<(std::ostream& out, const Data& data) {
    out << "Data (" << data.count() << " B) :";

    const auto begin { data.buffer().cbegin() };
    for (const byte b : std::vector<byte> { begin, begin + data.count() })
        out << ' ' << std::to_string(b);

    return out;
}

} // namespace Rbo

BOOST_AUTO_TEST_SUITE(Make)

BOOST_AUTO_TEST_CASE(Range) {
    const Data expected {
        std::vector<byte> {
            0, 0, 0, 12, 'H', 'o', 'w', ' ', 'a', 'r', 'e', ' ', 'y', 'o', 'u', '?', 5, 15
        }
    };
    const std::string msg { "How are you?" };

    SessionDataFactory factory;
    factory.makeRange(msg, 5, 15);

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_CASE(Possibilities) {
    const Data expected {
        std::vector<byte> {
            0, 1, 0, 5, 'C', 'h', 'o', 'i', 'x', 4, 0, 0, 4, 'Z', 'e', 'r', 'o', 1, 0, 3, 'O', 'n', 'e', 2, 0, 3, 'T', 'w', 'o', 4, 0, 4, 'F', 'o', 'u', 'r'
        }
    };
    const OptionsList arg { { 0, "Zero" }, { 1, "One" }, { 2, "Two" }, { 4, "Four" } };

    SessionDataFactory factory;
    factory.makePossibilities("Choix", arg);

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_CASE(Infos) {
    Data expected { std::vector<byte> { 2, 4 } };

    const json default_limits = json::object({
        { "min", StatValueLimits::min() }, { "max", StatValueLimits::max() }
    });
    const json infos = json::object({
        { "inventories", {
              { "inv1", { { "A", 5 }, { "B", -4 } } },
              { "inv2", { { "A", -1 } } }
        } },
        { "inventoriesMaxCapacity", { { "inv1", nullptr }, { "inv2", 55 } } },
        { "stats", {
              { "a", { { "hidden", false }, { "main", false }, { "limits", default_limits }, { "value", 1 } } },
              { "b", { { "hidden", false }, { "main", true }, { "limits", default_limits }, { "value", 2 } } }
        } }
    });

    expected.put(infos.dump());

    const byte player_id { 4 };
    const PlayerStateChanges changes {
        { { "a", Stat { 1, StatLimits {}, false, false } }, { "b", Stat { 2, StatLimits {}, false, true } } },
        {
            { "inv1", { { "A", 5 }, { "B", -4 } } },
            { "inv2", { { "A", -1 } } }
        },
        { { "inv1", {} }, { "inv2", 55 } }
    };

    SessionDataFactory factory;
    factory.makeInfos(player_id, changes);

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_CASE(BattleInit) {
    Data expected { std::vector<byte> { 8, 0 } };
    expected.put(R"({"A":{"hp":45,"skill":16},"B":{"hp":46,"skill":17}})");

    const std::unordered_map<std::string, EnemyDescriptor> enemies {
        { "EnemyA", { 45, 16 } },
        { "EnemyB", { 46, 17 } }
    };
    const GroupDescriptor group {
        { "A", "EnemyA" },
        { "B", "EnemyB" }
    };

    SessionDataFactory factory;
    factory.makeBattleInit(group, enemies);

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_CASE(GlobalStat) {
    const Data expected {
        std::vector<byte> {
            3, 0, 4, 't', 'e', 's', 't', 0, 0, 0x1, 0xD4, 0, 0, 0x7, 0xE4, 0, 0, 0, 0, 0, 1
        }
    };

    SessionDataFactory factory;
    factory.makeGlobalStat("test", Stat { 0, { 468, 2020 }, false, true });

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_SUITE_END()
