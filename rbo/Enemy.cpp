#include <Rbo/Enemy.hpp>

namespace Rbo {

std::vector<std::string> namesOf(const GroupDescriptor& group) {
    std::vector<std::string> names;
    names.resize(group.size());

    std::transform(group.cbegin(), group.cend(), names.begin(), [](const auto& e) {
        return e.first;
    });

    return names;
}

Enemy::Enemy(const EnemyDescriptor& descriptor)
    : stats_ { std::vector<std::string> { "hp", "skill" } }
{
    stats_.setLimits("hp", 0, std::numeric_limits<int>::max());
    stats_.setLimits("skill", 0, std::numeric_limits<int>::max());

    stats_.set("hp", descriptor.hp);
    stats_.set("skill", descriptor.skill);
}

EnemyPtr Enemy::create(const EnemyDescriptor& enemy) {
    struct AccessibleEnemy : Enemy {
        AccessibleEnemy(const EnemyDescriptor& enemy) : Enemy { enemy } {}
    };

    return std::make_shared<AccessibleEnemy>(enemy);
}

void Enemy::hit(const int dmg) {
    if (dmg < 0)
        throw std::invalid_argument { "Les dégâts ne peuvent pas être négatifs" };

    stats_.change("hp", -dmg);
}

void Enemy::heal(const int hp) {
    if (hp < 0)
        throw std::invalid_argument { "Les soins ne peuvent pas être négatifs" };

    stats_.change("hp", hp);
}

void Enemy::buff(const int bonus) {
    if (bonus < 0)
        throw std::invalid_argument { "Les bonus de dégâts ne peuvent pas être négatifs" };

    stats_.change("skill", bonus);
}

void Enemy::unbuff(const int malus) {
    if (malus < 0)
        throw std::invalid_argument { "Les malus de dégâts ne peuvent pas être négatifs" };

    stats_.change("skill", -malus);
}

} // namespace Rbo
