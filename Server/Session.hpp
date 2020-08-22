#ifndef SESSION_HPP
#define SESSION_HPP

#include "AsioCommon.hpp"

#include "Game.hpp"
#include "Player.hpp"

namespace Rbo {

class GameBuilder;
class Data;
class Gameplay;

enum struct ReplyValidity : byte;

struct NoPlayerRemaining : std::runtime_error {
    NoPlayerRemaining() : std::runtime_error { "Tous les joueurs se sont déconnectés" } {}
};

struct UnknownPlayer : std::logic_error {
    UnknownPlayer(const byte id)
        : std::logic_error { "Joueur " + std::to_string(id) + " inconnu" } {}
};

struct CancelledRequest : std::exception {
    const char* what() const noexcept override;
};

struct InvalidIDs : std::logic_error {
    std::vector<byte> expectedIDs;

    InvalidIDs(const std::vector<byte>& expected_ids)
        : std::logic_error { "Invalid IDs" }, expectedIDs { expected_ids } {}

    virtual const char* what() const noexcept override;
};

struct Particpant {
    std::string name;
    tcp::socket socket;
};

using Participants = std::map<byte, Particpant>;
using ConstConnections = std::map<byte, const tcp::socket*>;

class Session {
private:
    io::io_context::strand executor_;
    spdlog::logger& logger_;
    StatsManager stats_;
    std::map<byte, Player> players_;
    std::map<byte, tcp::socket> connections_;
    Game game_;
    byte leader_;
    std::map<byte, PlayerState> last_states_;
    bool first_state_;
    word current_scene_;
    std::atomic_bool running_;

    const GameBuilder& game_builer_;

    void initPlayers(Gameplay&);
    void initPlayer(Gameplay&, Player&);
    void restaurePlayer(const byte, const PlayerState&);

    Next playScene(Gameplay&, const word);
    void end(Participants&);

    void removePlayer(const byte);

    tcp::socket& connection(const byte);

public:
    friend void confirmController(const io::mutable_buffer&);
    friend struct RangeController;

    Session(io::io_context&, const GameBuilder&);

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    bool operator==(const Session&) const = delete;

    void start(Participants&, const std::string& = "", const bool = false);

    void stop() { running_ = false; }
    bool running() const { return running_; }
    void reset();

    std::string checkpoint(const std::string&) const;

    const Game& game() const { return game_; }

    StatsManager& stats() { return stats_; }
    const StatsManager& stats() const { return stats_; }

    const GameBuilder& gameBuilder() const { return game_builer_; }

    Replies request(const byte, const Data&, ReplyController, const bool);
    void sendTo(const byte, const Data&);
    void sendToAll(const Data&);

    void disconnect(const byte, const bool = false);

    byte leader() const { return leader_; }
    void switchLeader(const byte);

    Player& player(const byte);
    const Player& player(const byte) const;

    Players players();
    ConstPlayers players() const;
    std::size_t count() const { return players_.size(); }
    bool playersRemaining() const { return !players_.empty(); }

    const std::map<byte, PlayerState>& lastStates() const { return last_states_; }
    PlayerStateChanges getChanges(const byte);
};

} // namespace Rbo

#endif // SESSION_HPP
