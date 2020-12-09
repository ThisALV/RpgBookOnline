#ifndef GAMEPLAY_HPP
#define GAMEPLAY_HPP

#include <Rbo/Common.hpp>

namespace Rbo {

struct RestProperties;
class Session;

class Gameplay {
private:
    Session& ctx_;
    std::map<byte, PlayerCache> players_cache_;

public:
    Gameplay(Session& ctx) : ctx_ { ctx } {}

    void initCache(const byte player_id);

    StatsManager& global();
    const Game& game() const;
    const RestProperties& rest() const;

    // Throw : IntroductionCheckpoint, GameBuilder::save() implementation exceptions
    std::string checkpoint(const std::string& name, const word scene) const;

    Player& player(const byte id);
    std::vector<byte> players() const;
    OptionsList names() const;
    std::size_t count() const;

    // Throw : UninitializedLeader
    byte leader() const;
    // Throw : NoPlayerRemaining
    void switchLeader(const byte leader);
    void voteForLeader();

    // ask - Throw : NoPlayerRemaining
    Replies askReply(const byte target, const std::string& msg, const byte min, const byte max, const bool wait = true);
    Replies askReply(const byte target, const std::string& msg, const OptionsList& options, const bool wait = true);
    Replies askConfirm(const byte target, const bool wait = true);
    Replies askYesNo(const byte target, const std::string& question, const bool wait = true);

    PlayerCheckingResult checkPlayer(const byte id); // Throw : NoPlayerRemaining
    bool checkGame();

    // print - Throw : NoPlayerRemaining
    void print(const std::string& msg, const byte target = ALL_PLAYERS);

    // send - Throw : NoPlayerRemaining
    void sendGlobalStat(const std::string& stat_name);
    void sendPlayerUpdate(const byte player_id);
    void sendBattleInit(const GroupDescriptor& enemies_group_descriptor);
    void sendBattleAtk(const byte atk_player, const std::string& enemies_name, const int dmgs);
    void sendBattleEnd();
};

} // namespace Rbo

#endif // GAMEPLAY_HPP
