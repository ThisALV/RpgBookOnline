#include <Rbo/Server/InstructionsProvider.hpp>

#include <Rbo/Player.hpp>
#include <Rbo/Game.hpp>
#include <Rbo/Enemy.hpp>

namespace Rbo::Server {

void InstructionsProvider::initGameAPI() {
    sol::usertype<RollResult> roll_result_type { ctx_.new_usertype<RollResult>("RollResult") };
    roll_result_type["dices"] = &RollResult::dices;
    roll_result_type["bonus"] = &RollResult::bonus;
    roll_result_type["total"] = &RollResult::total;

    sol::usertype<DicesRoll> dices_roll_type { ctx_.new_usertype<DicesRoll>("DicesRoll") };
    dices_roll_type["dices"] = &DicesRoll::dices;
    dices_roll_type["bonus"] = &DicesRoll::bonus;
    dices_roll_type["min"] = &DicesRoll::min;
    dices_roll_type["max"] = &DicesRoll::max;
    dices_roll_type[sol::metatable_key][sol::meta_function::call] = &DicesRoll::operator();

    sol::usertype<RestProperties> rest_type { ctx_.new_usertype<RestProperties>("RestProperties") };
    rest_type["givables"] = sol::readonly(&RestProperties::givables);
    rest_type["availables"] = sol::readonly(&RestProperties::availables);

    sol::usertype<StatLimits> limits_type { ctx_.new_usertype<StatLimits>("StatLimits") };
    limits_type["min"] = &StatLimits::min;
    limits_type["max"] = &StatLimits::max;

    sol::usertype<StatsManager> stats_type { ctx_.new_usertype<StatsManager>("StatsManager") };
    stats_type["get"] = &StatsManager::get;
    stats_type["change"] = &StatsManager::change;
    stats_type["set"] = &StatsManager::set;
    stats_type["setLimits"] = &StatsManager::setLimits;
    stats_type["limits"] = &StatsManager::limits;
    stats_type["setHidden"] = &StatsManager::setHidden;
    stats_type["hidden"] = &StatsManager::hidden;
    stats_type["setMain"] = &StatsManager::setMain;
    stats_type["main"] = &StatsManager::main;
    stats_type["has"] = &StatsManager::has;

    sol::usertype<Inventory> inv_type { ctx_.new_usertype<Inventory>("Inventory") };
    inv_type["add"] = &Inventory::add;
    inv_type["consume"] = &Inventory::consume;
    inv_type["size"] = &Inventory::size;
    inv_type["count"] = &Inventory::count;
    inv_type["has"] = &Inventory::has;
    inv_type["limited"] = &Inventory::limited;
    inv_type["capacity"] = &Inventory::capacity;
    inv_type["setCapacity"] = &Inventory::setCapacity;

    sol::usertype<Player> player_type { ctx_.new_usertype<Player>("Player") };
    player_type["same"] = &Player::same;
    player_type["id"] = &Player::id;
    player_type["name"] = &Player::name;
    player_type["add"] = &Player::add;
    player_type["consume"] = &Player::consume;
    player_type["inventory"] = sol::resolve<Inventory&(const std::string&)>(&Player::inventory);
    player_type["stats"] = sol::resolve<StatsManager&()>(&Player::stats);

    const std::initializer_list<std::pair<std::string_view, EventEffect::ItemsChanges>> results {
            { "Ok", EventEffect::ItemsChanges::Ok },
            { "InventoryFull", EventEffect::ItemsChanges::InvFull },
            { "ItemEmpty", EventEffect::ItemsChanges::ItemEmpty }
    };
    ctx_.new_enum<EventEffect::ItemsChanges>("SimulationResult", results);

    sol::usertype<EventEffect> effect_type { ctx_.new_usertype<EventEffect>("EventEffect") };
    effect_type["apply"] = &EventEffect::apply;
    effect_type["simulateItemsChanges"] = &EventEffect::simulateItemsChanges;

    sol::usertype<Enemy> enemy_type { ctx_.new_usertype<Enemy>("Enemy") };
    enemy_type["alive"] = &Enemy::alive;
    enemy_type["hp"] = &Enemy::hp;
    enemy_type["skill"] = &Enemy::skill;
    enemy_type["hit"] = &Enemy::hit;
    enemy_type["heal"] = &Enemy::heal;
    enemy_type["buff"] = &Enemy::buff;
    enemy_type["unbuff"] = &Enemy::unbuff;

    sol::usertype<EnemiesGroup> group { ctx_.new_usertype<EnemiesGroup>("EnemiesGroup") };
    group["defeated"] = &EnemiesGroup::defeated;
    group["lastPos"] = &EnemiesGroup::lastPos;
    group["get"] = sol::overload(
            sol::resolve<Enemy&(const std::string&)>(&EnemiesGroup::get),
            sol::resolve<Enemy&(const byte)>(&EnemiesGroup::get)
    );
    group["current"] = sol::resolve<Enemy&()>(&EnemiesGroup::current);
    group["currentPos"] = &EnemiesGroup::currentPos;
    group["currentName"] = &EnemiesGroup::currentName;
    group["goTo"] = &EnemiesGroup::goTo;
    group["next"] = &EnemiesGroup::next;
    group["nextAlive"] = sol::overload(
            [](EnemiesGroup& self) -> Enemy& { return self.nextAlive(); },
            &EnemiesGroup::nextAlive
    );

    sol::usertype<Game> game_type { ctx_.new_usertype<Game>("Game") };
    game_type["name"] = sol::readonly(&Game::name);
    game_type["voteOnLeaderDeath"] = sol::readonly(&Game::voteOnLeaderDeath);
    game_type["voteLeader"] = sol::readonly(&Game::voteLeader);
    game_type["rest"] = sol::readonly(&Game::rest);
    game_type["effect"] = &Game::effect;
    game_type["enemy"] = &Game::enemy;
    game_type["group"] = &Game::group;

    sol::usertype<PlayerCheckingResult> check_result_type { ctx_.new_usertype<PlayerCheckingResult>("CheckingResult") };
    check_result_type["alive"] = sol::readonly(&PlayerCheckingResult::alive);
    check_result_type["leaderSwitch"] = sol::readonly(&PlayerCheckingResult::leaderSwitch);
    check_result_type["sessionEnd"] = sol::readonly(&PlayerCheckingResult::sessionEnd);

    ctx_["namesOf"] = namesOf;
}

}