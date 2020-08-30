#ifndef ENEMY_HPP
#define ENEMY_HPP

#include "StatsManager.hpp"

namespace Rbo {

class Enemy {
private:
    StatsManager stats_;

public:
    Enemy(const EnemyDescriptor&);

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

using EnemiesGroup = std::map<byte, Enemy>;

} // namespace Rbo

#endif // ENEMY_HPP
