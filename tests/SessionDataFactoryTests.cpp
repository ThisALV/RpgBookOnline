#define BOOST_TEST_MODULE SessionDataFactory

#include <iomanip>
#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>
#include <Rbo/Game.hpp>
#include <Rbo/SessionDataFactory.hpp>

using namespace Rbo;

using json = nlohmann::json;

namespace Rbo {

std::ostream& operator<<(std::ostream& out, const Data& data) {
    out << "Data (" << data.count() << " B) :" << std::hex << std::setw(4) << std::showbase << std::internal << std::setfill('0');

    const auto begin { data.buffer().cbegin() };
    for (const byte b : std::vector<byte> { begin, begin + data.count() })
        out << ' ' << int { b };

    return out << std::dec << std::setw(0) << std::noshowbase << std::internal;
}

} // namespace Rbo

BOOST_AUTO_TEST_SUITE(Make)

BOOST_AUTO_TEST_CASE(Range) {
    const Data expected {
        std::vector<byte> {
            0, 0, 255, 0, 12, 'H', 'o', 'w', ' ', 'a', 'r', 'e', ' ', 'y', 'o', 'u', '?', 5, 15
        }
    };

    SessionDataFactory factory;
    factory.makeRange(ALL_PLAYERS, "How are you?", 5, 15);

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_CASE(Options) {
    const Data expected {
        std::vector<byte> {
            0, 1, 0, 0, 5, 'C', 'h', 'o', 'i', 'x', 4, 0, 4, 'Z', 'e', 'r', 'o', 0, 3, 'O', 'n', 'e', 0, 3, 'T', 'w', 'o', 0, 4, 'F', 'o', 'u', 'r'
        }
    };
    const OptionsList arg { "Zero", "One", "Two", "Four" };

    SessionDataFactory factory;
    factory.makeOptions(0, "Choix", arg);

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_CASE(MoreThan256Options) {
    OptionsList arg;
    arg.resize(OPTIONS_LIMIT + 1);

    SessionDataFactory factory;
    BOOST_CHECK_THROW(factory.makeOptions(0, "Choix", arg), TooManyOptions);
}

BOOST_AUTO_TEST_CASE(DiceRoll) {
    const Data expected {
        std::vector<byte> {
            0, 4, 254, 0, 9, 'A', ' ', 'm', 'e', 's', 's', 'a', 'g', 'e', 3, 0xff, 0xff, 0xff, 0xff, 3,
            0, 2, 3, 4,
            1, 1, 6, 5,
            254, 5, 4, 1
        }
    };

    DiceRollResults results {
        { 0, {} }, { 1, {} }, { 254, {} }
    };
    results.at(0).dices = { 2, 3, 4 };
    results.at(1).dices = { 1, 6, 5 };
    results.at(254).dices = { 5, 4, 1 };

    SessionDataFactory factory;
    factory.makeDiceRoll(ACTIVE_PLAYERS, "A message", 3, -1, std::move(results));

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_CASE(PlayerUpdates) {
    Data expected { std::vector<byte> { 2, 4 } };

    const json default_limits = json::object({
        { "min", StatsValueLimits::min() }, { "max", StatsValueLimits::max() }
    });
    const json update = json::object({
        { "death", nullptr },
        { "inventories", {
              { "inv1", { { "A", 5 }, { "B", -4 } } },
              { "inv2", { { "A", -1 } } }
        } },
        { "capacities", { { "inv1", nullptr }, { "inv2", 55 } } },
        { "stats", {
              { "a", { { "hidden", true }, { "main", false } } },
              { "b", { { "hidden", false }, { "main", true }, { "limits", default_limits }, { "value", 2 } } },
              { "c", { { "hidden", true }, { "main", true } } }
        } }
    });

    expected.put(update.dump());

    const byte player_id { 4 };
    const PlayerUpdate changes {
        Death {},
        {
            { "a", Stat { 1, StatLimits {}, true, false } },
            { "b", Stat { 2, StatLimits {}, false, true } },
            { "c", Stat { 3, StatLimits {}, true, true } }
        },
        {
            { "inv1", { { "A", 5 }, { "B", -4 } } },
            { "inv2", { { "A", -1 } } }
        },
        { { "inv1", {} }, { "inv2", 55 } }
    };

    SessionDataFactory factory;
    factory.makePlayerUpdate(player_id, changes);

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_CASE(BattleInit) {
    Data expected { std::vector<byte> { 8, 0 } };
    const json group_data {
        { "1", { { "name", "B" }, { "hp", 46 }, { "skill", 17 } } },
        { "3", { { "name", "A" }, { "hp", 45 }, { "skill", 16 } } }
    };
    expected.put(group_data.dump());

    Game ctx;
    ctx.enemies = {
        { "EnemyA", { 45, 16 } },
        { "EnemyB", { 46, 17 } }
    };

    const GroupDescriptor group {
        { 3, { "A", "EnemyA" } },
        { 1, { "B", "EnemyB" } }
    };

    SessionDataFactory factory;
    factory.makeBattleInit(group, ctx);

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_SUITE(GlobalStat)

BOOST_AUTO_TEST_CASE(Visible) {
    const Data expected {
        std::vector<byte> {
            3, 0, 4, 't', 'e', 's', 't', 0, 1, 0, 0, 0x1, 0xD4, 0, 0, 0x7, 0xE4, 0, 0, 0, 0
        }
    };

    SessionDataFactory factory;
    factory.makeGlobalStat("test", Stat { 0, { 468, 2020 }, false, true });

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_CASE(HiddenAndMain) {
    const Data expected {
        std::vector<byte> {
            3, 0, 4, 't', 'e', 's', 't', 1, 1
        }
    };

    SessionDataFactory factory;
    factory.makeGlobalStat("test", Stat { 0, { 468, 2020 }, true, true });

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_CASE(HiddenNotMain) {
    const Data expected {
        std::vector<byte> {
            3, 0, 4, 't', 'e', 's', 't', 1, 0
        }
    };

    SessionDataFactory factory;
    factory.makeGlobalStat("test", Stat { 0, { 468, 2020 }, true, false });

    BOOST_CHECK_EQUAL(expected, factory.data());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
