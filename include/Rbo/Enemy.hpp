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

    Enemy(const EnemyDescriptor& descriptor);
public:
    static EnemyPtr create(const EnemyDescriptor& descriptor);

    Enemy(const Enemy&) = delete;
    Enemy& operator=(const Enemy&) = delete;

    bool operator==(const Enemy&) const = delete;

    bool alive() const { return stats_.get("hp") != 0; }
    uint hp() const { return stats_.get("hp"); }
    uint skill() const { return stats_.get("skill"); }

    void hit(const int dmg);
    void heal(const int hp);
    void buff(const int bonus);
    void unbuff(const int malus);
};

using EnemiesGroup = std::unordered_map<std::string, EnemyPtr>;

std::vector<std::string> namesOf(const GroupDescriptor&);

} // namespace Rbo

#endif // ENEMY_HPP
