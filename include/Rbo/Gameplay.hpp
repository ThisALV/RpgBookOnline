#ifndef GAMEPLAY_HPP
#define GAMEPLAY_HPP

#include <Rbo/Common.hpp>

namespace Rbo {

struct RestProperties;
class Session;

class Gameplay {
private:
    Session& ctx_;

public:
    Gameplay(Session& ctx) : ctx_ { ctx } {}

    StatsManager& global();
    const Game& game() const;
    const RestProperties& rest() const;

    std::string checkpoint(const std::string& name, const word scene);

    Player& player(const byte id);
    std::vector<byte> players() const;
    OptionsList names() const;
    std::size_t count() const;

    byte leader() const;
    // Throw : NePlayerRemaining
    void switchLeader(const byte leader);
    void voteForLeader();

    // ask - Throw : NoPlayerRemaining
    Replies askReply(const byte target, const std::string& msg, const byte min, const byte max, const bool wait = true);
    Replies askReply(const byte target, const std::string& msg, const OptionsList& options, const bool wait = true);
    Replies askConfirm(const byte target, const bool wait = true);
    Replies askYesNo(const byte target, const std::string& question, const bool wait = true);

    PlayerCheckingResult checkPlayer(const byte id);
    bool checkGame();

    // print - Throw : NoPlayerRemaining
    void print(const std::string& msg, const byte target = ALL_PLAYERS);

    // send et end - Throw : NoPlayerRemaining
    void sendGlobalStat(const std::string& statName);
    void sendInfos(const byte playerInfosID);
    void sendBattleInfos(const GroupDescriptor& enemiesGroupDescriptor);
    void sendBattleAtk(const byte playerAtk, const std::string& enemiesName, const int dmgs);
    void sendBattleEnd();
};

} // namespace Rbo

#endif // GAMEPLAY_HPP
