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

    std::string checkpoint(const std::string&, const word);

    Player& player(const byte);
    std::vector<byte> players() const;
    std::size_t count() const;

    byte leader() const;
    // Throw : NePlayerRemaining
    void switchLeader(const byte);
    void voteForLeader();

    // ask - Throw : NoPlayerRemaining
    Replies askReply(const byte, const byte, const byte, const bool = true);
    Replies askReply(const byte, const std::vector<byte>&, const bool = true);
    Replies askConfirm(const byte, const bool wait = true);
    Replies askYesNo(const byte, const bool wait = true);

    PlayerCheckingResult checkPlayer(const byte);
    bool checkGame();

    // print - Throw : NoPlayerRemaining
    void print(const std::string&, const byte = ALL_PLAYERS);
    void printOptions(const OptionsList&, const byte = ALL_PLAYERS);
    void printOptions(const std::vector<std::string>&, const byte = ALL_PLAYERS,
                      const byte = 1);
    void printYesNo(const byte = ALL_PLAYERS);

    // send et end - Throw : NoPlayerRemaining
    void sendGlobalStat(const std::string&);
    void sendInfos(const byte);
    void sendBattleInfos(const GroupDescriptor&);
    void sendBattleAtk(const byte, const std::string&, const int);
    void endBattle();
};

} // namespace Rbo

#endif // GAMEPLAY_HPP
