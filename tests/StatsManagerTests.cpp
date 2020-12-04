#define BOOST_TEST_MODULE StatsManager

#include <Rbo/Tests/TestsCommon.hpp>

#include <boost/test/unit_test.hpp>
#include <Rbo/StatsManager.hpp>

using namespace Rbo;

BOOST_TEST_DONT_PRINT_LOG_VALUE(Stats)

BOOST_AUTO_TEST_SUITE(Ctor)

const Stat default_stat {
    0, StatLimits { StatValueLimits::min(), StatValueLimits::max() }, false
};

BOOST_AUTO_TEST_CASE(StatsNames) {
    const StatsManager manager { std::vector<std::string> { "a", "b", "c", "d", "e" } };
    const Stats expected {
        { "a", default_stat },
        { "b", default_stat },
        { "c", default_stat },
        { "d", default_stat },
        { "e", default_stat }
    };

    BOOST_CHECK_EQUAL(expected, manager.raw());
}

BOOST_AUTO_TEST_SUITE_END()

struct SetFixture {
    inline static const StatLimits STAT_LIMITS { -500, 500 };

    StatsManager manager { std::vector<std::string> { "a" } };

    SetFixture() {
        manager.setLimits("a", -500, 500);

        assert(manager.limits("a") == STAT_LIMITS);
    }
};

template<int initialValue = 100> struct ChangeFixture : SetFixture {
    ChangeFixture() : SetFixture {} {
        manager.set("a", initialValue);
    }
};

BOOST_AUTO_TEST_SUITE(Set)

BOOST_FIXTURE_TEST_CASE(InLimits, SetFixture)
{
    const StatsValues expected_values { { "a", -177 } };

    manager.set("a", -177);

    BOOST_CHECK_EQUAL(StatsWrapper { expected_values }, StatsWrapper { manager.values() });
}

BOOST_FIXTURE_TEST_CASE(OffLimitsMax, SetFixture) {
    const StatsValues expected_values { { "a", 500 } };

    manager.set("a", 755);

    BOOST_CHECK_EQUAL(manager.get("a"), 500);
}

BOOST_FIXTURE_TEST_CASE(OffLimitsMin, SetFixture) {
    const StatsValues expected_values { { "a", -500 } };

    manager.set("a", -833);

    BOOST_CHECK_EQUAL(manager.get("a"), -500);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Change, *boost::unit_test::depends_on { "Set" })

BOOST_FIXTURE_TEST_CASE(InLimits, ChangeFixture<>)
{
    const StatsValues expected_values { { "a", -450 } };

    manager.change("a", -550);

    BOOST_CHECK_EQUAL(StatsWrapper { expected_values }, StatsWrapper { manager.values() });
}

BOOST_FIXTURE_TEST_CASE(OffLimitsMax, ChangeFixture<>) {
    const StatsValues expected_values { { "a", 500 } };

    manager.change("a", 410);

    BOOST_CHECK_EQUAL(manager.get("a"), 500);
}

BOOST_FIXTURE_TEST_CASE(OffLimitsMin, ChangeFixture<>) {
    const StatsValues expected_values { { "a", -500 } };

    manager.change("a", -610);

    BOOST_CHECK_EQUAL(manager.get("a"), -500);
}

BOOST_AUTO_TEST_SUITE_END()

struct UnknownStatFixture {
    StatsManager manager;
};

BOOST_FIXTURE_TEST_SUITE(UnknownStatTests, UnknownStatFixture)

BOOST_AUTO_TEST_CASE(Get) { BOOST_CHECK_THROW(manager.get(""), UnknownStat); }
BOOST_AUTO_TEST_CASE(Set) { BOOST_CHECK_THROW(manager.set("", 0), UnknownStat); }
BOOST_AUTO_TEST_CASE(Change) { BOOST_CHECK_THROW(manager.change("", 0), UnknownStat); }
BOOST_AUTO_TEST_CASE(Limits) { BOOST_CHECK_THROW(manager.limits(""), UnknownStat); }
BOOST_AUTO_TEST_CASE(SetLimits) { BOOST_CHECK_THROW(manager.setLimits("", 0, 0), UnknownStat); }
BOOST_AUTO_TEST_CASE(Hidden) { BOOST_CHECK_THROW(manager.hidden(""), UnknownStat); }
BOOST_AUTO_TEST_CASE(SetHidden) { BOOST_CHECK_THROW(manager.setHidden("", false), UnknownStat); }

BOOST_AUTO_TEST_SUITE_END()
