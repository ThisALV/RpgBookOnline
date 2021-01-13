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

struct EnemyNotFound : std::logic_error {
    explicit EnemyNotFound(const std::string& name) : std::logic_error { "The enemy \"" + name + "\" isn't in this group" } {}
};

struct NotEnoughEnemies : std::logic_error {
    explicit NotEnoughEnemies(const std::size_t pos)
            : std::logic_error { "There is less than " + std::to_string(pos + 1) + " in the group's queue" } {}
};

struct NoMoreEnemies : std::logic_error {
    NoMoreEnemies() : std::logic_error { "All enemies are already killed in this group" } {}
};

class EnemiesGroup {
private:
    EnemiesQueue queue_;
    EnemiesQueue::iterator current_;

    std::size_t checkName(const std::string& name) const;
    std::size_t checkPos(const std::size_t pos) const;
public:
    friend void testNameChecking(const EnemiesGroup& group, const std::string& ctx_name);
    friend void testPosChecking(const EnemiesGroup& group, const byte pos_in_queue);

    EnemiesGroup(const std::string& group_name, const Game& ctx);

    EnemiesGroup(const EnemiesGroup&) = delete;
    EnemiesGroup& operator=(const EnemiesGroup&) = delete;

    EnemiesGroup(EnemiesGroup&&) = default;
    EnemiesGroup& operator=(EnemiesGroup&&) = default;

    bool operator==(const EnemiesGroup&) const = delete;

    std::size_t count() const { return queue().size(); }
    bool defeated() const;
    std::size_t lastPos() const { return queue_.size() - 1; }

    Enemy& get(const std::string& enemy_name);
    const Enemy& get(const std::string& enemy_name) const;

    Enemy& get(const std::size_t pos_in_queue);
    const Enemy& get(const std::size_t pos_in_queue) const;
    Enemy& current() { return *current_; }
    const Enemy& current() const { return *current_; }

    std::size_t currentPos() const { return current_ - queue().cbegin(); }
    const std::string& currentName() const { return current().name(); }

    Enemy& goTo(const size_t pos_in_queue);
    std::tuple<bool, Enemy*> next();
    std::tuple<bool, Enemy*> nextAlive(const bool self_included = true);

    const EnemiesQueue& queue() const { return queue_; }
};

std::vector<std::string> namesOf(const GroupDescriptor&);

} // namespace Rbo

#endif // ENEMY_HPP
