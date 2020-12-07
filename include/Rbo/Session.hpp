#ifndef SESSION_HPP
#define SESSION_HPP

#include <Rbo/AsioCommon.hpp>

#include <Rbo/Game.hpp>
#include <Rbo/Player.hpp>

namespace Rbo {

class GameBuilder;
class Data;
class Gameplay;

enum struct ReplyValidity : byte;

struct NoPlayerRemaining : std::runtime_error {
    NoPlayerRemaining() : std::runtime_error { "All players disconnected" } {}
};

struct UnknownPlayer : std::logic_error {
    UnknownPlayer(const byte id) : std::logic_error { "Unknown player " + std::to_string(id) } {}
};

struct CanceledRequest : std::exception {
    const char* what() const noexcept override;
};

enum struct ParticipantsValidity {
    Ok, UnknownPlayer, LessMembers
};

struct InvalidIDs : std::logic_error {
    std::vector<byte> expectedIDs;
    ParticipantsValidity errType;

    std::string msg;

    InvalidIDs(const std::vector<byte>& expectedIDs, const ParticipantsValidity errType);

    virtual const char* what() const noexcept override { return msg.data(); }
};

struct IntroductionCheckpoint : std::logic_error {
    IntroductionCheckpoint() : std::logic_error { "Saving during intro scene " + std::to_string(INTRO) } {}
};

struct Particpant {
    std::string name;
    tcp::socket socket;
};

using Participants = std::map<byte, Particpant>;
using ConstConnections = std::map<byte, const tcp::socket*>;

class Session {
private:
    static std::string initStatMsg(const DiceFormula& initFormula, const std::string& statName, const int statValue);

    io::io_context::strand executor_;
    spdlog::logger& logger_;
    StatsManager stats_;
    std::map<byte, Player> players_;
    std::map<byte, tcp::socket> connections_;
    Game game_;
    byte leader_;
    word current_scene_;
    std::atomic_bool running_;

    const GameBuilder& game_builer_;

    void initPlayers(Gameplay& interface);
    void initPlayer(Gameplay& interface, Player& target);
    void restaurePlayer(const byte targetID, const PlayerState& previousState);

    Next playScene(Gameplay& interface, const word sceneID);
    void end(Participants& initialParticipantsData);

    void removePlayer(const byte targetID);

    tcp::socket& connection(const byte playerID);
    void logPlayerError(const byte playerID, const std::string& msg);

public:
    friend void confirmController(const io::mutable_buffer&);
    friend struct RangeController;

    Session(io::io_context& executionCtx, const GameBuilder& gameBuilder);

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    bool operator==(const Session&) const = delete;

    void start(Participants& initialParticipantsData, const std::string& checkpt = "", const bool missing_participants = false);

    void stop() { running_ = false; }
    bool running() const { return running_; }
    void reset();

    std::string checkpoint(const std::string& generic_name, const word sceneID) const;

    const Game& game() const { return game_; }

    StatsManager& stats() { return stats_; }
    const StatsManager& stats() const { return stats_; }

    const GameBuilder& gameBuilder() const { return game_builer_; }

    Replies request(const byte targetsID, const Data& request_data, ReplyController controller, const bool wait_all_players);
    void sendTo(const byte target, const Data& data);
    void sendToAll(const Data& data);

    void disconnect(const byte target, const bool is_crash = false);

    byte leader() const { return leader_; }
    void switchLeader(const byte new_leader);

    Player& player(const byte id);
    const Player& player(const byte id) const;

    std::vector<byte> ids() const;
    Players players();
    ConstPlayers players() const;
    std::size_t count() const { return players_.size(); }
    bool playersRemaining() const { return !players_.empty(); }
};

} // namespace Rbo

namespace std {

template<typename Output>
Output& operator<<(Output& out, const Rbo::Replies& replies) {
    out << '[';
    for (const auto [id, reply] : replies)
        out << ' ' << std::to_string(id) << "->" << std::to_string(reply);

    out << " ]";
    return out;
}

}

#endif // SESSION_HPP
