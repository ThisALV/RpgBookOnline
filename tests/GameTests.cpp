#define BOOST_TEST_MODULE Game

#include <Rbo/Tests/TestsCommon.hpp>

#include <boost/test/unit_test.hpp>
#include <Rbo/Game.hpp>

using namespace Rbo;

namespace Rbo {

std::ostream& operator<<(std::ostream& out, const EventEffect::ItemsChanges result) {
    return out << static_cast<int>(result);
}

std::ostream& operator<<(std::ostream& out, const Game::Error result) {
    return out << static_cast<int>(result);
}

} // namespace Rbo

BOOST_AUTO_TEST_SUITE(EventEffectTests)

BOOST_AUTO_TEST_CASE(Apply) {
    const ItemsList inventories {
        { "inv1", std::vector<std::string> { "A", "B" } },
        { "inv2", std::vector<std::string> { "A" } }
    };
    Player target { 1, "TestPlayer", std::vector<std::string> { "a", "b" }, inventories, {} };

    target.stats().setLimits("a", -10, 20);
    target.stats().set("a", 18);
    target.add("inv1", "B", 5);

    const EventEffect effect {
        { { "a", -99 }, { "b", -5 } },
        { { itemEntry("inv1", "B"), -5 }, { itemEntry("inv2", "A"), 1 } }
    };

    effect.apply(target);
    const StatsValues expected_stats { { "a", -10 }, { "b", -5 } };

    PlayerInventories expected_inventories {
        { "inv1", Inventory { std::vector<std::string> { "A", "B" } } },
        { "inv2", Inventory { std::vector<std::string> { "A" } } }
    };
    expected_inventories.at("inv2").add("A", 1);

    BOOST_CHECK_EQUAL(StatsWrapper { expected_stats }, StatsWrapper { target.stats().values() });
    BOOST_CHECK_EQUAL(InventoriesWrapper { expected_inventories },
                      InventoriesWrapper { target.inventories() });
}

BOOST_AUTO_TEST_CASE(ApplyLimitExceeded) {
    const ItemsList inventories { { "inv1", std::vector<std::string> { "A", "B", "C", "D" } } };
    Player target { 1, "TestPlayer", {}, inventories, {} };
    target.inventory("inv1").setMaxSize(20);

    target.add("inv1", "A", 5);
    target.add("inv1", "B", 5);
    target.add("inv1", "C", 5);
    target.add("inv1", "D", 5);

    const EventEffect effect {
        {}, {
            { itemEntry("inv1", "A"), 5 }, { itemEntry("inv1", "B"), -5 },
            { itemEntry("inv1", "C"), 5 }, { itemEntry("inv1", "D"), -5 }
        }
    };

    effect.apply(target);
    PlayerInventories expected_inventories {
        { "inv1", Inventory { std::vector<std::string> { "A", "B", "C", "D" }, 20 } }
    };

    Inventory& inv1 { expected_inventories.at("inv1") };
    inv1.add("A", 10);
    inv1.add("C", 10);

    BOOST_CHECK_EQUAL(expected_inventories, target.inventories());
}

struct SimulateItemsChangeFixture {
    const ItemsList inventories {
        { "inv1", std::vector<std::string> { "A", "B" } },
        { "inv2", std::vector<std::string> { "A" } }
    };
    Player target { 1, "TestPlayer", {}, inventories, {} };

    SimulateItemsChangeFixture() {
        target.inventory("inv1").setMaxSize(10);

        target.add("inv1", "A", 1);
        target.add("inv1", "B", 2);
        target.add("inv2", "A", 5);
    }
};

BOOST_FIXTURE_TEST_SUITE(SimulateItemsChange, SimulateItemsChangeFixture)

BOOST_AUTO_TEST_CASE(ItemEmpty) {
    const EventEffect effect {
        {}, {
            { itemEntry("inv1", "A"), -1 }, { itemEntry("inv1", "B"), 2 },
            { itemEntry("inv2", "A"), -6 }
        }
    };

    const EventEffect::ItemsChanges result { effect.simulateItemsChanges(target) };

    BOOST_CHECK_EQUAL(EventEffect::ItemsChanges::ItemEmpty, result);
}

BOOST_AUTO_TEST_CASE(InvFull) {
    const EventEffect effect {
        {}, {
            { itemEntry("inv1", "A"), 4 }, { itemEntry("inv1", "B"), 6 },
            { itemEntry("inv2", "A"), -5 }
        }
    };

    const EventEffect::ItemsChanges result { effect.simulateItemsChanges(target) };

    BOOST_CHECK_EQUAL(EventEffect::ItemsChanges::InvFull, result);
}

BOOST_AUTO_TEST_CASE(Ok) {
    const EventEffect effect {
        {}, {
            { itemEntry("inv1", "A"), -1 }, { itemEntry("inv1", "B"), 8 },
            { itemEntry("inv2", "A"), -5 }
        }
    };

    const EventEffect::ItemsChanges result { effect.simulateItemsChanges(target) };

    BOOST_CHECK_EQUAL(EventEffect::ItemsChanges::Ok, result);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(ConditionTests)

struct TestFixture {
    Player target { 1, "TestPlayer", std::vector<std::string> { "a" }, {}, {} };

    TestFixture() {
        target.stats().set("a", 5);
    }
};

BOOST_FIXTURE_TEST_SUITE(Test, TestFixture)

BOOST_AUTO_TEST_CASE(True) {
    const Condition condition { "a", "==", 5 };

    BOOST_CHECK(condition.test(target.stats()));
}

BOOST_AUTO_TEST_CASE(False) {
    const Condition condition { "a", ">", 5 };

    BOOST_CHECK(!condition.test(target.stats()));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(GameTests)

BOOST_AUTO_TEST_SUITE(Valid)

BOOST_AUTO_TEST_CASE(InitialInventoriesFull) {
    const InventoryDescriptor inventory {
        std::optional<DiceFormula> {},
        std::vector<std::string> { "A" },
        InventoryContent {
            { { itemEntry("inv1", "A"), 2 }, { itemEntry("inv1", "B"), 3 } }
        }
    };

    Game game;
    game.playerInventories.insert({ "inv1", inventory });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::InitialInventories, validity.back());
}

BOOST_AUTO_TEST_CASE(InitialInventoriesUnknownItem) {
    const InventoryDescriptor inventory {
        DiceFormula { 2, 2 },
        std::vector<std::string> { "A", "B" },
        InventoryContent { { { "A", 2 }, { "B", 3 } } }
    };

    Game game;
    game.playerInventories.insert({ "inv1", inventory });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::InitialInventories, validity.back());
}

BOOST_AUTO_TEST_CASE(BonusesUnknownStat) {
    const InventoryDescriptor inventory {
        std::optional<DiceFormula> {},
        std::vector<std::string> { "A" },
        InventoryContent {}
    };

    Game game;
    game.playerInventories.insert({ "inv1", inventory });
    game.bonuses.insert({ itemEntry("inv1", "A"), ItemBonus { "a", 3 } });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::Bonuses, validity.back());
}

BOOST_AUTO_TEST_CASE(BonusesUnknownItem) {
    const InventoryDescriptor inventory {
        std::optional<DiceFormula> {},
        std::vector<std::string> { "B" },
        InventoryContent {}
    };

    Game game;
    game.playerStats.insert({ "a", StatDescriptor {} });
    game.playerInventories.insert({ "inv1", inventory });
    game.bonuses.insert({ itemEntry("inv1", "A"), ItemBonus { "a", 3 } });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::Bonuses, validity.back());
}

BOOST_AUTO_TEST_CASE(EffectsUnknownStat) {
    const InventoryDescriptor inventory {
        std::optional<DiceFormula> {},
        std::vector<std::string> { "A" },
        InventoryContent {}
    };
    const EventEffect effect {
        { { "a", -5 } },
        { { itemEntry("inv1", "A"), 1 } }
    };

    Game game;
    game.playerInventories.insert({ "inv1", inventory });
    game.eventEffects.insert({ "evt1", effect });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::Effects, validity.back());
}

BOOST_AUTO_TEST_CASE(EffectsUnknownItem) {
    const InventoryDescriptor inventory {
        std::optional<DiceFormula> {},
        std::vector<std::string> { "A" },
        InventoryContent {}
    };
    const EventEffect effect {
        { { "a", -5 } },
        { { itemEntry("inv2", "A"), 1 } }
    };

    Game game;
    game.playerStats.insert({ "a", StatDescriptor {} });
    game.playerInventories.insert({ "inv1", inventory });
    game.eventEffects.insert({ "evt1", effect });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::Effects, validity.back());
}

BOOST_AUTO_TEST_CASE(GroupsUnknownEnemy) {
    Game game;
    game.groups.insert({ "1", { { 1, { "1", "EnemyA" } }, { 2, { "2", "EnemyB" } } } });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::Groups, validity.back());
}

BOOST_AUTO_TEST_CASE(RestGivablesUnknownItem) {
    Game game;
    game.rest.givables.push_back(itemEntry("inv1", "A"));

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::RestGivables, validity.back());
}

BOOST_AUTO_TEST_CASE(RestAvailablesUnknownItem) {
    Game game;
    game.eventEffects.insert({ "evt1", EventEffect {} });
    game.rest.availables.insert({ itemEntry("inv1", "A"), "evt1" });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::RestAvailables, validity.back());
}

BOOST_AUTO_TEST_CASE(RestAvailablesUnknownEffect) {
    const InventoryDescriptor inventory {
        std::optional<DiceFormula> {},
        std::vector<std::string> { "A" },
        InventoryContent {}
    };

    Game game;
    game.playerInventories.insert({ "inv1", inventory });
    game.eventEffects.insert({ "evt1", EventEffect {} });
    game.rest.availables.insert({ itemEntry("inv1", "A"), "evt2" });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::RestAvailables, validity.back());
}

BOOST_AUTO_TEST_CASE(DeathConditionsUnknownStat) {
    Game game;
    game.deathConditions.push_back(Condition { "a", "!=", 5 });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::DeathConditions, validity.back());
}

BOOST_AUTO_TEST_CASE(DeathConditionsUnknownOperator) {
    Game game;
    game.playerStats.insert({ "a", StatDescriptor {} });
    game.deathConditions.push_back(Condition { "a", "===", 5 });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::DeathConditions, validity.back());
}

BOOST_AUTO_TEST_CASE(GameEndConditionsUnknownStat) {
    Game game;
    game.gameEndConditions.push_back(Condition { "1", ">=", 5 });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::GameEndConditions, validity.back());
}

BOOST_AUTO_TEST_CASE(GameEndConditionsUnknownOperator) {
    Game game;
    game.globalStats.insert({ "1", StatDescriptor {} });
    game.gameEndConditions.push_back(Condition { "1", "===", 5 });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(1, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::GameEndConditions, validity.back());
}

BOOST_AUTO_TEST_CASE(EffectUnknownStatAndBonusUnknownItem) {
    Game game;
    game.globalStats.insert({ "a", {} });
    game.eventEffects.insert({ "eff1", EventEffect { { { "none", 0 } }, {} } });
    game.bonuses.insert({ itemEntry("inv0", "none"), ItemBonus { "a", 0 } });

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK_EQUAL(2, validity.size());
    BOOST_CHECK_EQUAL(Game::Error::Effects, validity.back());
    BOOST_CHECK_EQUAL(Game::Error::Bonuses, validity.front());
}

BOOST_AUTO_TEST_CASE(Ok) {
    StatsDescriptors global {
        { "1", { DiceFormula { 2, 0 }, StatLimits { -15, 15 }, false, false, true } },
        { "2", {} }
    };
    StatsDescriptors player {
        { "a", { DiceFormula { 1, 0 }, StatLimits { -10, 10 }, true, true, false } },
        { "b", {} }
    };
    InventoriesDescriptors inventories {
        { "inv1", { DiceFormula { 1, 2 }, { "A", "B" }, InventoryContent { { "A", 3 } } } },
        { "inv2", { std::optional<DiceFormula> {}, { "A" }, InventoryContent {} } }
    };

    ItemsBonuses bonuses {
        { itemEntry("inv1", "B"), ItemBonus { "a", 8 } },
        { itemEntry("inv2", "A"), ItemBonus { "b", -5 } }
    };
    Effects effects {
        { "evt1", { { { "b", 99 } }, {} } },
        { "evt2", { {}, { { itemEntry("inv1", "A"), 1 } } } }
    };
    std::unordered_map<std::string, EnemyDescriptor> enemies {
        { "EnemyA", { 45, 15 } },
        { "EnemyB", { 46, 46 } }
    };
    std::unordered_map<std::string, GroupDescriptor> groups {
        { "GroupA", { { 1, { "1", "EnemyA" } }, { 2, { "2", "EnemyA" } } } },
        { "GroupB", { { 2, { "1", "EnemyB" } }, { 2, { "2", "EnemyB" } } } }
    };
    RestProperties rest {
        { itemEntry("inv1", "A"), itemEntry("inv2", "A") },
        { { itemEntry("inv1", "A"), "evt2" } }
    };
    std::vector<Condition> deathConditions {
        Condition { "b", "==", 3 }, Condition { "b", "==", 0 }
    };
    std::vector<Condition> gameEndConditions {
        Condition { "2", "==", 3 }, Condition { "2", "==", 0 }
    };

    const Game game {
        "TestGame",
        std::move(global), std::move(player), std::move(inventories), std::move(bonuses),
        std::move(effects), std::move(enemies), std::move(groups), std::move(rest),
        std::move(deathConditions), std::move(gameEndConditions), true, true
    };

    const std::vector<Game::Error> validity { game.validity() };
    BOOST_CHECK(validity.empty());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
