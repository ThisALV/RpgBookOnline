#ifndef ENEMY_HPP
#define ENEMY_HPP

#include <Rbo/StatsManager.hpp>

namespace Rbo {

class Enemy {
private:
    std::string name_;
    StatsManager stats_;

public:
    Enemy(const std::string& name, const EnemyDescriptor& descriptor);

    Enemy(const Enemy&) = delete;
    Enemy& operator=(const Enemy&) = delete;

    Enemy(Enemy&&) = default;
    Enemy& operator=(Enemy&&) = default;

    bool operator==(const Enemy&) const = delete;

    const std::string& name() const { return name_; }
    bool alive() const { return stats_.get("hp") != 0; }
    uint hp() const { return stats_.get("hp"); }
    uint skill() const { return stats_.get("skill"); }

    void hit(const int dmg);
    void heal(const int hp);
    void buff(const int bonus);
    void unbuff(const int malus);
};

struct NegativeModifier : std::logic_error {
    explicit NegativeModifier(const std::string& reason) : std::logic_error { reason } {}
};

struct SameEnemiesName : std::logic_error {
    explicit SameEnemiesName(const std::string& ctx_name) : std::logic_error { "Name \"" + ctx_name + "\" already took" } {}
};

struct TooManyEnemies : std::logic_error {
    TooManyEnemies() : std::logic_error { "There cannot be more than " + std::to_string(255) + " enemies in one group" } {}
};

struct EnemyNotFound : std::logic_error {
    explicit EnemyNotFound(const std::string& name) : std::logic_error { "The enemy \"" + name + "\" isn't in this group" } {}
};

struct NotEnoughEnemies : std::logic_error {
    explicit NotEnoughEnemies(const byte pos) : std::logic_error { "There is less than " + std::to_string(pos + 1) + " in the group's queue" } {}
};

struct NoMoreEnemies : std::logic_error {
    NoMoreEnemies() : std::logic_error { "All enemies are already killed in this group" } {}
};


class EnemiesGroup {
private:
    std::unordered_map<std::string_view, Enemy> enemies_;
    std::vector<std::string_view> queue_;
    byte current_;

    void checkName(const std::string_view name) const;
    void checkPos(const byte pos) const;
public:
    friend void testNameChecking(const EnemiesGroup& group, const std::string_view ctx_name);
    friend void testPosChecking(const EnemiesGroup& group, const byte pos_in_queue);

    static constexpr std::size_t LIMIT { std::numeric_limits<byte>::max() };

    EnemiesGroup(const std::string& group_name, const Game& ctx);

    EnemiesGroup(const EnemiesGroup&) = delete;
    EnemiesGroup& operator=(const EnemiesGroup&) = delete;

    EnemiesGroup(EnemiesGroup&&) = default;
    EnemiesGroup& operator=(EnemiesGroup&&) = default;

    bool operator==(const EnemiesGroup&) const = delete;

    bool defeated() const;
    byte lastPos() const { return queue_.size() - 1; }

    Enemy& get(const std::string_view enemy_name);
    const Enemy& get(const std::string_view enemy_name) const;

    Enemy& get(const byte pos_in_queue);
    const Enemy& get(const byte pos_in_queue) const;
    Enemy& current();
    const Enemy& current() const;

    byte currentPos() const { return current_; }
    const std::string_view& currentName() const { return queue_.at(currentPos()); }

    Enemy& goTo(const byte pos_in_queue);
    Enemy& next();
    Enemy& nextAlive(const bool self_included = true);

    const std::unordered_map<std::string_view, Enemy>& enemies() const { return enemies_; }
    const std::vector<std::string_view>& queue() const { return queue_; }
};

std::vector<std::string> namesOf(const GroupDescriptor&);

} // namespace Rbo

#endif // ENEMY_HPP
