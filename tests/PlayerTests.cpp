#define BOOST_TEST_MODULE Player

#include <Rbo/Tests/TestsCommon.hpp>

#include <boost/test/unit_test.hpp>

using namespace Rbo;

BOOST_TEST_DONT_PRINT_LOG_VALUE(ItemsBonus)
BOOST_TEST_DONT_PRINT_LOG_VALUE(Stats)
BOOST_TEST_DONT_PRINT_LOG_VALUE(InventoryContent)

BOOST_AUTO_TEST_SUITE(PlayerTests)

BOOST_AUTO_TEST_SUITE(Ctor)

const Stat default_stat {
    0, StatLimits { StatsValueLimits::min(), StatsValueLimits::max() }, false
};

BOOST_AUTO_TEST_CASE(Informations) {
    const byte id { 1 };
    const std::string name { "TestPlayer" };
    const ItemsList inventories {
        { "inv1", std::vector<std::string> { "A", "B" } },
        { "inv2", std::vector<std::string> { "A" } }
    };
    const ItemsBonus bonuses {
        { "inv1/A", { "a", 2 } }, { "inv2/A", { "c", -5 } }
    };

    const Player player {
        id, name, std::vector<std::string> { "a", "b", "c", "d", "e" }, inventories, bonuses
    };
    const Stats expected_stats {
        { "a", default_stat },
        { "b", default_stat },
        { "c", default_stat },
        { "d", default_stat },
        { "e", default_stat }
    };
    const PlayerInventories expected_inventories {
        { "inv1", Inventory { std::vector<std::string> { "A", "B" } } },
        { "inv2", Inventory { std::vector<std::string> { "A" } } }
    };

    BOOST_CHECK_EQUAL(id, player.id());
    BOOST_CHECK_EQUAL(name, player.name());
    BOOST_CHECK_EQUAL(expected_stats, player.stats().raw());
    BOOST_CHECK_EQUAL(expected_inventories, player.inventories());
    BOOST_CHECK_EQUAL(bonuses, player.statsBonus());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Alive)

BOOST_AUTO_TEST_CASE(IsAlive) {
    const Player p { 0, "", {}, {}, {} };

    BOOST_CHECK(p.alive());
    BOOST_CHECK_THROW(p.death(), PlayerNotDead);
}

BOOST_AUTO_TEST_CASE(IsKilled) {
    Player p { 0, "", {}, {}, {} };
    p.kill("Random reason");

    BOOST_CHECK(!p.alive());
    BOOST_CHECK_EQUAL(p.death(), "Random reason");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Items, *boost::unit_test::depends_on { "InventoryTests" })

struct AddFixture {
    Player player {
        1, "TestPlayer", std::vector<std::string> { "A" },
        ItemsList { { "inv1", std::vector<std::string> { "A" } } },
        ItemsBonus { { "inv1/A", { "A", -3 } } }
    };

    AddFixture() {
        assert(player.inventories().count("inv1") == 1);
        player.inventory("inv1").setCapacity(10);
    }
};

struct ConsumeFixture : AddFixture {
    ConsumeFixture() : AddFixture {} {
        player.add("inv1", "A", 5);
    }
};

BOOST_AUTO_TEST_CASE(AddNegativeQty) {
    Player player { 0, "", {}, { { "1", { "A" } } }, {} };

    BOOST_CHECK_THROW(player.add("1", "A", -1), InvalidQty);
}

BOOST_AUTO_TEST_CASE(ConsumeNegativeQty) {
    Player player { 0, "", {}, { { "1", { "A" } } }, {} };

    BOOST_CHECK_THROW(player.consume("1", "A", -1), InvalidQty);
}

BOOST_AUTO_TEST_CASE(AddUnknownInventory) {
    BOOST_CHECK_THROW(Player(0, "", {}, {}, {}).add("", "", 0), UnknownInventory);
}

BOOST_AUTO_TEST_CASE(AddUnknownItem) {
    BOOST_CHECK_THROW(Player(0, "", {}, { { "inv1", {} } }, {}).add("inv1", "", 0), UnknownItem);
}

BOOST_FIXTURE_TEST_CASE(Add, AddFixture) {
    const bool done { player.add("inv1", "A", 5) };
    const StatsValue expected_stats { { "A", -15 } };
    PlayerInventories expected_inventories {
        { "inv1", Inventory { std::vector<std::string> { "A" }, 10 } }
    };
    expected_inventories.at("inv1").add("A", 5);

    BOOST_CHECK(done);
    BOOST_CHECK_EQUAL(StatsValueWrapper { expected_stats }, StatsValueWrapper { player.stats().values() });
    BOOST_CHECK_EQUAL(expected_inventories, player.inventories());
}

BOOST_AUTO_TEST_CASE(ConsumeUnknownInventory) {
    BOOST_CHECK_THROW(Player(0, "", {}, {}, {}).consume("", "", 0), UnknownInventory);
}

BOOST_AUTO_TEST_CASE(ConsumeUnknownItem) {
    BOOST_CHECK_THROW(Player(0, "", {}, { { "inv1", {} } }, {}).consume("inv1", "", 0), UnknownItem);
}

BOOST_FIXTURE_TEST_CASE_WITH_DECOR(Consume, ConsumeFixture,
                                   *boost::unit_test::depends_on { "PlayerTests/Items/Add" })
{
    const bool done { player.consume("inv1", "A", 2) };
    const StatsValue expected_stats { { "A", -9 } };
    PlayerInventories expected_inventories {
        { "inv1", Inventory { std::vector<std::string> { "A" }, 10 } }
    };
    expected_inventories.at("inv1").add("A", 3);

    BOOST_CHECK(done);
    BOOST_CHECK_EQUAL(StatsValueWrapper { expected_stats }, StatsValueWrapper { player.stats().values() });
    BOOST_CHECK_EQUAL(expected_inventories, player.inventories());
}

BOOST_FIXTURE_TEST_CASE(AddInventoryFull, AddFixture) {
    const bool done { player.add("inv1", "A", 11) };
    const StatsValue expected_stats { { "A", 0 } };
    const PlayerInventories expected_inventories {
        { "inv1", Inventory { std::vector<std::string> { "A" }, 10 } }
    };

    BOOST_CHECK(!done);
    BOOST_CHECK_EQUAL(StatsValueWrapper { expected_stats }, StatsValueWrapper { player.stats().values() });
    BOOST_CHECK_EQUAL(expected_inventories, player.inventories());
}

BOOST_FIXTURE_TEST_CASE_WITH_DECOR(ConsumeItemEmpty, ConsumeFixture,
                                   *boost::unit_test::depends_on { "PlayerTests/Items/Add" })
{
    const bool done { player.consume("inv1", "A", 10) };
    const StatsValue expected_stats { { "A", -15 } };
    PlayerInventories expected_inventories {
        { "inv1", Inventory { std::vector<std::string> { "A" }, 10 } }
    };
    expected_inventories.at("inv1").add("A", 5);

    BOOST_CHECK(!done);
    BOOST_CHECK_EQUAL(StatsValueWrapper { expected_stats }, StatsValueWrapper { player.stats().values() });
    BOOST_CHECK_EQUAL(expected_inventories, player.inventories());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(InventoryTests)

BOOST_AUTO_TEST_SUITE(Ctor)

BOOST_AUTO_TEST_CASE(NegativeCapacity) {
    BOOST_CHECK_THROW(Inventory(std::vector<std::string>(), -1), InvalidCapacity);
}

BOOST_AUTO_TEST_CASE(ItemsWithCapacity) {
    const Inventory inventory { std::vector<std::string> { "A", "B" }, 2 };
    const InventoryContent expected_content { { "A", 0 }, { "B", 0 } };

    BOOST_CHECK(inventory.limited());
    BOOST_CHECK_EQUAL(*inventory.capacity(), 2);
    BOOST_CHECK_EQUAL(inventory.content(), expected_content);
}

BOOST_AUTO_TEST_CASE(ItemsWithoutCapacity) {
    const Inventory inventory { std::vector<std::string> { "A", "B" } };
    const InventoryContent expected_content { { "A", 0 }, { "B", 0 } };

    BOOST_CHECK(!inventory.limited());
    BOOST_CHECK_EQUAL(inventory.content(), expected_content);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(Size) {
    Inventory inventory { std::vector<std::string> { "A", "B" } };
    inventory.add("A", 55);
    inventory.add("B", 45);
    inventory.consume("A", 10);

    BOOST_CHECK_EQUAL(inventory.size(), 90);
}

struct SetCapacityFixture {
    Inventory inventory { std::vector<std::string> { "A", "B" }, 5 };

    SetCapacityFixture() {
        inventory.add("A", 1);
        inventory.add("B", 2);
    }
};

BOOST_FIXTURE_TEST_SUITE(SetCapacity, SetCapacityFixture)

BOOST_AUTO_TEST_CASE(NegativeCapacity) {
    BOOST_CHECK_THROW(inventory.setCapacity(-1), InvalidCapacity);
}

BOOST_AUTO_TEST_CASE(NotEnoughCapacity) {
    BOOST_CHECK(!inventory.setCapacity(2));
}

BOOST_AUTO_TEST_CASE(EnoughCapacity) {
    BOOST_CHECK(inventory.setCapacity({}));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
