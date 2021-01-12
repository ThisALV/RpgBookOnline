#include <Rbo/Enemy.hpp>

#include <Rbo/Game.hpp>

namespace Rbo {

std::vector<std::string> namesOf(const GroupDescriptor& group) {
    std::vector<std::string> names;
    names.resize(group.size());

    std::transform(group.cbegin(), group.cend(), names.begin(), [](const auto& e) {
        return e.first;
    });

    return names;
}

Enemy::Enemy(const std::string& name, const EnemyDescriptor& descriptor) : name_ { name }, stats_ { std::vector<std::string> { "hp", "skill" } } {
    stats_.setLimits("hp", 0, std::numeric_limits<int>::max());
    stats_.setLimits("skill", 0, std::numeric_limits<int>::max());

    stats_.set("hp", descriptor.hp);
    stats_.set("skill", descriptor.skill);
}

void Enemy::hit(const int dmg) {
    if (dmg < 0)
        throw NegativeModifier { "Damages must be positive (else, it's damage)" };

    stats_.change("hp", -dmg);
}

void Enemy::heal(const int hp) {
    if (hp < 0)
        throw NegativeModifier { "Heals HP must be positive (else, it's healing)" };

    stats_.change("hp", hp);
}

void Enemy::buff(const int bonus) {
    if (bonus < 0)
        throw NegativeModifier { "Damages buff must be positive (else, it's unbuff)" };

    stats_.change("skill", bonus);
}

void Enemy::unbuff(const int malus) {
    if (malus < 0)
        throw NegativeModifier { "Damages unbuff must be positive (else, it's buff)" };

    stats_.change("skill", -malus);
}

EnemiesGroup::EnemiesGroup(const std::string& group_name, const Game& ctx) : current_ { 0 } {
    const GroupDescriptor& descriptor { ctx.group(group_name) };

    if (descriptor.size() > LIMIT)
        throw TooManyEnemies {};

    queue_.reserve(descriptor.size());
    for (const auto& enemy : descriptor) {
        const auto& [ctxName, genericName] { enemy.second };

        if (enemies_.count(ctxName) == 1)
            throw SameEnemiesName { ctxName };

        queue_.push_back(ctxName);
        enemies_.insert({ ctxName, Enemy { ctxName, ctx.enemy(genericName) } });
    }

    assert(enemies_.size() == queue_.size());
}

void EnemiesGroup::checkName(const std::string& name) const {
    if (enemies_.count(name) == 0)
        throw EnemyNotFound { name };
}

void EnemiesGroup::checkPos(const byte pos) const {
    if (pos >= queue().size())
        throw NotEnoughEnemies { pos };
}

bool EnemiesGroup::defeated() const {
    return std::none_of(enemies_.cbegin(), enemies_.cend(), [](const auto& e) -> bool {
        return e.second.alive();
    });
}

Enemy& EnemiesGroup::get(const std::string& enemy_name) {
    checkName(enemy_name);
    return enemies_.at(enemy_name);
}

const Enemy& EnemiesGroup::get(const std::string& enemy_name) const {
    checkName(enemy_name);
    return enemies_.at(enemy_name);
}

Enemy& EnemiesGroup::get(const byte pos_in_queue) {
    checkPos(pos_in_queue);
    return enemies_.at(queue_.at(pos_in_queue));
}

const Enemy& EnemiesGroup::get(const byte pos_in_queue) const {
    checkPos(pos_in_queue);
    return enemies_.at(queue_.at(pos_in_queue));
}

Enemy& EnemiesGroup::current() {
    assert(queue_.size() > current_);
    return enemies_.at(queue_.at(current_));
}

const Enemy& EnemiesGroup::current() const {
    assert(queue_.size() > current_);
    return enemies_.at(queue_.at(current_));
}

Enemy& EnemiesGroup::goTo(const byte pos_in_queue) {
    checkPos(pos_in_queue);

    current_ = pos_in_queue;
    return enemies_.at(currentName());
}

Enemy& EnemiesGroup::next() {
    current_ = currentPos() == lastPos() ? 0 : currentPos() + 1;
    return current();
}

Enemy& EnemiesGroup::nextAlive(const bool self_included) {
    if (defeated())
        throw NoMoreEnemies {};

    byte next;
    if (self_included) {
        next = currentPos();
    } else {
        next = currentPos() == lastPos() ? byte { 0 } : currentPos() + 1;
    }

    while (!get(next).alive()) {
        if (next == lastPos())
            next = 0;
        else
            next++;
    }

    assert(enemies().at(queue().at(next)).alive());

    current_ = next;
    return current();
}

} // namespace Rbo
