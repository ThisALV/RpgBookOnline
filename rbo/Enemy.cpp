#include <Rbo/Enemy.hpp>

#include <Rbo/Game.hpp>

namespace Rbo {

std::vector<std::string> namesOf(const GroupDescriptor& group) {
    std::vector<std::string> names;
    names.resize(group.size());

    std::transform(group.cbegin(), group.cend(), names.begin(), [](const EnemyDescriptorBinding& e) {
        return e.genericName;
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

EnemiesGroup::EnemiesGroup(const std::string& group_name, const Game& ctx) {
    const GroupDescriptor& descriptor { ctx.group(group_name) };

    queue_.reserve(descriptor.size());
    for (const auto& [ctxName, genericName] : descriptor) {
        const std::string& uniqueName { ctxName }; // Nécessaire pour la capture dans une lambda
        const auto b { queue().cbegin() };
        const auto e { queue().cend() };

        if (std::find_if(b, e, [&uniqueName](const Enemy& e) { return e.name() == uniqueName; }) != e)
            throw SameEnemiesName { ctxName };

        queue_.push_back(Enemy { ctxName, ctx.enemy(genericName) });
    }

    current_ = queue_.begin();
}

std::size_t EnemiesGroup::checkName(const std::string& name) const {
    const auto b { queue().cbegin() };
    const auto e { queue().cend() };

    const auto result { std::find_if(b, e, [&name](const Enemy& e) { return e.name() == name; }) };
    if (result == e)
        throw EnemyNotFound { name };

    return static_cast<byte>(result - b);
}

std::size_t EnemiesGroup::checkPos(const std::size_t pos) const {
    if (pos >= queue().size())
        throw NotEnoughEnemies { pos };

    return pos;
}

bool EnemiesGroup::defeated() const {
    return std::none_of(queue().cbegin(), queue().cend(), [](const auto& e) {
        return e.alive();
    });
}

Enemy& EnemiesGroup::get(const std::string& enemy_name) {
    return queue_.at(checkName(enemy_name));
}

const Enemy& EnemiesGroup::get(const std::string& enemy_name) const {
    return queue().at(checkName(enemy_name));
}

Enemy& EnemiesGroup::get(const std::size_t pos_in_queue) {
    return queue_.at(checkPos(pos_in_queue));
}

const Enemy& EnemiesGroup::get(const std::size_t pos_in_queue) const {
    return queue().at(checkPos(pos_in_queue));
}

Enemy& EnemiesGroup::goTo(const size_t pos_in_queue) {
    current_ += checkPos(pos_in_queue);

    return *current_;
}

Enemy& EnemiesGroup::next() {
    if (current_ == (queue().cend() - 1))
        current_ = queue_.begin();
    else
        current_++;

    return current();
}

Enemy& EnemiesGroup::nextAlive(const bool self_included) {
    if (defeated())
        throw NoMoreEnemies {};

    EnemiesQueue::iterator next;
    if (self_included)
        next = current_;
    else
        next = current_ == (queue_.cend() - 1) ? queue_.begin() : (current_ + 1);

    while (!next->alive()) {
        if (next == (queue_.cend() - 1))
            next = queue_.begin();
        else
            next++;
    }

    assert(next->alive());

    current_ = next;
    return current();
}

} // namespace Rbo
