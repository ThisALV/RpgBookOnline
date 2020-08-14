#ifndef GAMEPLAY_HPP
#define GAMEPLAY_HPP

#include "Common.hpp"

struct RestProperties;
class Session;

class Gameplay {
private:
    Session& ctx_;

public:
    Gameplay(Session& ctx) : ctx_ { ctx } {}

    StatsManager& global();
    const StatsManager& global() const;

    const Game& game() const;
    const RestProperties& rest() const;

    std::string checkpoint(const std::string&);

    Player& player(const byte);
    const Player& player(const byte) const;

    Players players();
    ConstPlayers players() const;
    std::size_t count() const;

    byte leader() const;
    // Throw : NePlayerRemaining
    void switchLeader(const byte);
    void voteForLeader();

    // ask - Throw : NoPlayerRemaining
    Replies askReply(const byte, const byte, const byte, const bool wait = false);
    Replies askReply(const byte, const std::vector<byte>&, const bool wait = false);
    Replies askConfirm(const byte, const bool wait = false);
    Replies askYesNo(const byte, const bool wait = false);

    PlayerCheckingResult checkPlayer(const byte);
    bool checkGame();

    // print - Throw : NoPlayerRemaining
    void print(const std::string&, const byte target = ALL_PLAYERS);
    void printOptions(const OptionsList&, const byte target = ALL_PLAYERS);
    void printOptions(const std::vector<std::string>&, const byte target = ALL_PLAYERS,
                      const byte begin = 1);
    void printYesNo(const byte target = ALL_PLAYERS);

    // send et end - Throw : NoPlayerRemaining
    void sendGlobalStat(const std::string&);
    void sendInfos(const byte);
    void sendBattleInfos(const EnemyInitializers&);
    void sendBattleAtk(const byte, const byte, const s_byte);
    void endBattle();
};

#endif // GAMEPLAY_HPP
