#ifndef ENEMY_HPP
#define ENEMY_HPP

#include <memory>
#include <Rbo/StatsManager.hpp>

namespace Rbo {

class Enemy;

using EnemyPtr = std::shared_ptr<Enemy>;

class Enemy {
private:
    StatsManager stats_;

    Enemy(const EnemyDescriptor&);
public:
    static EnemyPtr create(const EnemyDescriptor&);

    Enemy(const Enemy&) = delete;
    Enemy& operator=(const Enemy&) = delete;

    bool operator==(const Enemy&) const = delete;

    bool alive() const { return stats_.get("hp") != 0; }
    uint hp() const { return stats_.get("hp"); }
    uint skill() const { return stats_.get("skill"); }

    void hit(const int);
    void heal(const int);
    void buff(const int);
    void unbuff(const int);
};

using EnemiesGroup = std::unordered_map<std::string, EnemyPtr>;

std::vector<std::string> namesOf(const GroupDescriptor&);

} // namespace Rbo

#endif // ENEMY_HPP
