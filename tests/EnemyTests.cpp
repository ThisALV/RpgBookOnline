#define BOOST_TEST_MODULE Enemy

#include <Rbo/Tests/TestsCommon.hpp>

#include <boost/test/unit_test.hpp>
#include <Rbo/Enemy.hpp>
#include <Rbo/Game.hpp>
#include <Rbo/Player.hpp>

namespace Rbo {

void testNameChecking(const EnemiesGroup& group, const std::string& name) {
    group.checkName(name);
}

void testPosChecking(const EnemiesGroup& group, const byte pos) {
    group.checkPos(pos);
}

} // namespace Rbo

BOOST_AUTO_TEST_SUITE(EnemyTests)

BOOST_AUTO_TEST_SUITE(Ctor)

BOOST_AUTO_TEST_CASE(DescriptorInLimits) {
    const EnemyDescriptor descriptor { 5, 99 };
    const Enemy enemy { "A", descriptor };

    BOOST_CHECK_EQUAL(enemy.name(), "A");
    BOOST_CHECK_EQUAL(enemy.hp(), 5);
    BOOST_CHECK_EQUAL(enemy.skill(), 99);
}

BOOST_AUTO_TEST_CASE(DescriptorDeadEnemy) {
    const EnemyDescriptor descriptor { -5, 99 };
    const Enemy enemy { "A", descriptor };

    BOOST_CHECK_EQUAL(enemy.name(), "A");
    BOOST_CHECK_EQUAL(enemy.hp(), 0);
    BOOST_CHECK_EQUAL(enemy.skill(), 99);
}

BOOST_AUTO_TEST_CASE(DescriptorNegativeSkill) {
    const EnemyDescriptor descriptor { 5, -99 };
    const Enemy enemy { "A", descriptor };

    BOOST_CHECK_EQUAL(enemy.name(), "A");
    BOOST_CHECK_EQUAL(enemy.hp(), 5);
    BOOST_CHECK_EQUAL(enemy.skill(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Hit)

BOOST_AUTO_TEST_CASE(NotKill) {
    Enemy enemy { "", EnemyDescriptor { 5, 0 } };

    enemy.hit(3);

    BOOST_CHECK_EQUAL(enemy.hp(), 2);
}

BOOST_AUTO_TEST_CASE(Kill) {
    Enemy enemy { "", EnemyDescriptor { 5, 0 } };

    enemy.hit(5);

    BOOST_CHECK(!enemy.alive());
}

BOOST_AUTO_TEST_CASE(Overkill) {
    Enemy enemy { "", EnemyDescriptor { 5, 0 } };

    enemy.hit(99);

    BOOST_CHECK(!enemy.alive());
}

BOOST_AUTO_TEST_CASE(NegativeDmg) {
    BOOST_CHECK_THROW(Enemy("", EnemyDescriptor{ 5, 0 }).hit(-1), NegativeModifier);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Heal)

BOOST_AUTO_TEST_CASE(Normal) {
    Enemy enemy { "", EnemyDescriptor { 5, 0 } };

    enemy.heal(3);

    BOOST_CHECK_EQUAL(enemy.hp(), 8);
}

BOOST_AUTO_TEST_CASE(NegativeHealing) {
    BOOST_CHECK_THROW(Enemy("", EnemyDescriptor{ 5, 0 }).heal(-1), NegativeModifier);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Buff)

BOOST_AUTO_TEST_CASE(Normal) {
    Enemy enemy { "", EnemyDescriptor { 0, 5 } };

    enemy.buff(3);

    BOOST_CHECK_EQUAL(enemy.skill(), 8);
}

BOOST_AUTO_TEST_CASE(Unbuff) {
    BOOST_CHECK_THROW(Enemy("", EnemyDescriptor{ 0, 5 }).buff(-1), NegativeModifier);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Unbuff)

BOOST_AUTO_TEST_CASE(Normal) {
    Enemy enemy { "", EnemyDescriptor { 0, 5 } };

    enemy.unbuff(3);

    BOOST_CHECK_EQUAL(enemy.skill(), 2);
}

BOOST_AUTO_TEST_CASE(ToNegative) {
    Enemy enemy { "", EnemyDescriptor { 0, 5 } };

    enemy.unbuff(10);

    BOOST_CHECK_EQUAL(enemy.skill(), 0);
}

BOOST_AUTO_TEST_CASE(Buff) {
    BOOST_CHECK_THROW(Enemy("", EnemyDescriptor{ 0, 5 }).unbuff(-1), NegativeModifier);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(EnemiesGroupTests)

BOOST_AUTO_TEST_SUITE(Ctor)

BOOST_AUTO_TEST_CASE(MoreThan255Enemies) {
    GroupDescriptor descriptor;
    for (std::size_t i { 0 }; i <= EnemiesGroup::LIMIT; i++)
        descriptor.insert({ i, { "", "" } });

    BOOST_CHECK_THROW(EnemiesGroup(descriptor, Game()), TooManyEnemies);
}

BOOST_AUTO_TEST_CASE(SameCtxNameTwice) {
    Game ctx;
    ctx.enemies = {
        { "A", {} }, { "B", {} }
    };

    const GroupDescriptor descriptor {
        { 3, EnemyDescriptorBinding { "1", "A" } },
        { 2, EnemyDescriptorBinding { "2", "A" } },
        { 1, EnemyDescriptorBinding { "1", "B" } }
    };

    BOOST_CHECK_THROW(EnemiesGroup(descriptor, ctx), SameEnemiesName);
}

BOOST_AUTO_TEST_CASE(Normal) {
    Game ctx;
    ctx.enemies = {
        { "A", {} }, { "B", {} }
    };

    const GroupDescriptor descriptor {
        { 3, EnemyDescriptorBinding { "3", "A" } },
        { 1, EnemyDescriptorBinding { "1", "A" } },
        { 2, EnemyDescriptorBinding { "2", "B" } }
    };

    const EnemiesGroup group { descriptor, ctx };
    const std::vector<std::string> expected_queue { "3", "2", "1" };

    BOOST_CHECK_EQUAL(StrVecWrapper { group.queue() }, StrVecWrapper { expected_queue });
    BOOST_CHECK_EQUAL(group.currentPos(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Defeated)

BOOST_AUTO_TEST_CASE(NoEnemiesAlive) {
    Game ctx;
    ctx.enemies = { { "A", { 0, 5 } } };

    const GroupDescriptor descriptor {
        { 1, EnemyDescriptorBinding { "1", "A" } },
        { 2, EnemyDescriptorBinding { "2", "A" } }
    };

    const EnemiesGroup group { descriptor, ctx };

    BOOST_CHECK(group.defeated());
}

BOOST_AUTO_TEST_CASE(AnyEnemiesAlive) {
    Game ctx;
    ctx.enemies = {
        { "A", { 0, 5 } },
        { "B", { 1, 5 } },
    };

    const GroupDescriptor descriptor {
        { 1, EnemyDescriptorBinding { "1", "A" } },
        { 2, EnemyDescriptorBinding { "2", "B" } },
        { 0, EnemyDescriptorBinding { "3", "A" } }
    };

    const EnemiesGroup group { descriptor, ctx };

    BOOST_CHECK(!group.defeated());
}

BOOST_AUTO_TEST_SUITE_END()

struct EnemiesFixture {
    Game ctx;
    const GroupDescriptor descriptor {
        { 1, EnemyDescriptorBinding { "1", "A" } },
        { 2, EnemyDescriptorBinding { "2", "B" } },
        { 0, EnemyDescriptorBinding { "3", "A" } }
    };

    EnemiesFixture() {
        ctx.enemies = {
            { "A", { 0, 5 } },
            { "B", { 1, 5 } },
        };
    }
};

BOOST_FIXTURE_TEST_SUITE(CheckName, EnemiesFixture)

BOOST_AUTO_TEST_CASE(Exists) {
    testNameChecking(EnemiesGroup { descriptor, ctx }, "1");
}

BOOST_AUTO_TEST_CASE(NotExists) {
    const EnemiesGroup group { descriptor, ctx };

    BOOST_CHECK_THROW(testNameChecking(group, "0"), EnemyNotFound);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(CheckPos, EnemiesFixture)

BOOST_AUTO_TEST_CASE(InRange) {
    testPosChecking(EnemiesGroup { descriptor, ctx }, 2);
}

BOOST_AUTO_TEST_CASE(OutRange) {
    const EnemiesGroup group { descriptor, ctx };

    BOOST_CHECK_THROW(testPosChecking(group, 3), NotEnoughEnemies);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(GetByPos, EnemiesFixture)

BOOST_AUTO_TEST_CASE(PosInRange) {
    const EnemiesGroup group { descriptor, ctx };

    const Enemy& enemy { group.get(1) };
    const EnemyDescriptor& descriptor { ctx.enemies.at("A") };

    BOOST_CHECK_EQUAL(enemy.name(), "1");
    BOOST_CHECK_EQUAL(enemy.hp(), descriptor.hp);
    BOOST_CHECK_EQUAL(enemy.skill(), descriptor.skill);
}

BOOST_AUTO_TEST_CASE(PosOutRange) {
    const EnemiesGroup group { descriptor, ctx };

    BOOST_CHECK_THROW(group.get(10), NotEnoughEnemies);
}

BOOST_AUTO_TEST_CASE(Current) {
    const EnemiesGroup group { descriptor, ctx };

    const Enemy& enemy { group.current() };
    const EnemyDescriptor& descriptor { ctx.enemies.at("B") };

    BOOST_CHECK_EQUAL(enemy.name(), "2");
    BOOST_CHECK_EQUAL(enemy.hp(), descriptor.hp);
    BOOST_CHECK_EQUAL(enemy.skill(), descriptor.skill);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(GoTo, EnemiesFixture)

BOOST_AUTO_TEST_CASE(InRange) {
    EnemiesGroup group { descriptor, ctx };

    const Enemy& enemy { group.goTo(2) };
    const EnemyDescriptor descriptor { ctx.enemies.at("A") };

    BOOST_CHECK_EQUAL(enemy.name(), "3");
    BOOST_CHECK_EQUAL(enemy.hp(), descriptor.hp);
    BOOST_CHECK_EQUAL(enemy.skill(), descriptor.skill);
}

BOOST_AUTO_TEST_CASE(OutRange) {
    EnemiesGroup group { descriptor, ctx };

    BOOST_CHECK_THROW(group.goTo(3), NotEnoughEnemies);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(Next, EnemiesFixture)

BOOST_AUTO_TEST_CASE(BeginOfQueue) {
    EnemiesGroup group { descriptor, ctx };

    const Enemy& enemy { group.next() };
    const EnemyDescriptor& descriptor { ctx.enemies.at("A") };

    BOOST_CHECK_EQUAL(enemy.name(), "1");
    BOOST_CHECK_EQUAL(enemy.hp(), descriptor.hp);
    BOOST_CHECK_EQUAL(enemy.skill(), descriptor.skill);
}

BOOST_AUTO_TEST_CASE(EndOfQueue) {
    EnemiesGroup group { descriptor, ctx };
    group.goTo(2);

    const Enemy& enemy { group.next() };
    const EnemyDescriptor& descriptor { ctx.enemies.at("B") };

    BOOST_CHECK_EQUAL(enemy.name(), "2");
    BOOST_CHECK_EQUAL(enemy.hp(), descriptor.hp);
    BOOST_CHECK_EQUAL(enemy.skill(), descriptor.skill);
}

BOOST_AUTO_TEST_CASE(Defeated) {
    const GroupDescriptor descriptor {
        { 1, EnemyDescriptorBinding { "1", "A" } },
        { 2, EnemyDescriptorBinding { "2", "A" } }
    };
    EnemiesGroup group { descriptor, ctx };

    const Enemy& enemy { group.next() };
    const EnemyDescriptor& e_descriptor { ctx.enemies.at("A") };

    BOOST_CHECK_EQUAL(enemy.name(), "1");
    BOOST_CHECK_EQUAL(enemy.hp(), e_descriptor.hp);
    BOOST_CHECK_EQUAL(enemy.skill(), e_descriptor.skill);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(NextAlive, EnemiesFixture)

BOOST_AUTO_TEST_CASE(Defeated) {
    const GroupDescriptor descriptor {
        { 1, EnemyDescriptorBinding { "1", "A" } },
        { 2, EnemyDescriptorBinding { "2", "A" } }
    };

    EnemiesGroup group { descriptor, ctx };

    BOOST_CHECK_THROW(group.nextAlive(), NoMoreEnemies);
}

BOOST_AUTO_TEST_CASE(NotDefeated) {
    const GroupDescriptor descriptor {
        { 10, EnemyDescriptorBinding { "1", "A" } },
        { 5, EnemyDescriptorBinding { "2", "A" } },
        { 2, EnemyDescriptorBinding { "3", "B" }},
        { -99, EnemyDescriptorBinding { "4", "A" } }
    };

    EnemiesGroup group { descriptor, ctx };
    const Enemy& enemy { group.nextAlive() };
    const EnemyDescriptor e_descriptor { ctx.enemies.at("B") };

    BOOST_CHECK_EQUAL(enemy.name(), "3");
    BOOST_CHECK_EQUAL(enemy.hp(), e_descriptor.hp);
    BOOST_CHECK_EQUAL(enemy.skill(), e_descriptor.skill);
}

BOOST_AUTO_TEST_CASE(BackToBeginning) {
    const GroupDescriptor descriptor {
        { 10, EnemyDescriptorBinding { "1", "A" } },
        { 5, EnemyDescriptorBinding { "2", "B" } },
        { 2, EnemyDescriptorBinding { "3", "A" }},
        { -99, EnemyDescriptorBinding { "4", "A" } }
    };

    EnemiesGroup group { descriptor, ctx };
    group.goTo(2);

    const Enemy& enemy { group.nextAlive() };
    const EnemyDescriptor e_descriptor { ctx.enemies.at("B") };

    BOOST_CHECK_EQUAL(enemy.name(), "2");
    BOOST_CHECK_EQUAL(enemy.hp(), e_descriptor.hp);
    BOOST_CHECK_EQUAL(enemy.skill(), e_descriptor.skill);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
