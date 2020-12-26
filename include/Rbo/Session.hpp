#ifndef SESSION_HPP
#define SESSION_HPP

#include <Rbo/AsioCommon.hpp>

#include <Rbo/Game.hpp>
#include <Rbo/Player.hpp>

namespace Rbo {

class GameBuilder;
class Data;
class Gameplay;

struct GameState;

enum struct ReplyValidity : byte;

struct NoPlayerRemaining : std::runtime_error {
    NoPlayerRemaining() : std::runtime_error { "All players disconnected" } {}
};

struct UnknownPlayer : std::logic_error {
    UnknownPlayer(const byte id) : std::logic_error { "Unknown player " + std::to_string(id) } {}
};

struct UninitializedLeader : std::logic_error {
    UninitializedLeader() : std::logic_error { "Game's leader isn't initialized yet" } {}
};

struct CanceledRequest : std::exception {
    const char* what() const noexcept override;
};

enum struct EntrantsValidity {
    Ok, UnknownPlayer, LessMembers
};

struct InvalidIDs : std::logic_error {
    std::vector<byte> expectedIDs;
    EntrantsValidity errType;

    std::string msg;

    InvalidIDs(const std::vector<byte>& expectedIDs, const EntrantsValidity errType);

    virtual const char* what() const noexcept override { return msg.data(); }
};

struct IntroductionCheckpoint : std::logic_error {
    IntroductionCheckpoint() : std::logic_error { "Saving during intro scene " + std::to_string(INTRO) } {}
};

struct Particpant {
    std::string name;
    tcp::socket socket;
};

using Entrants = std::map<byte, Particpant>;
using ConstConnections = std::map<byte, const tcp::socket*>;

// Nécessaire pour conserver les détails des lancés de dés et les envoyés aux joueurs par la suite
struct DiceRollsDetails {
    std::unordered_map<std::string, RollResult> globalStats;
    std::map<byte, std::unordered_map<std::string, RollResult>> playersStats;
    std::map<byte, std::unordered_map<std::string, RollResult>> playersInvsCapacity;
};

class Session {
private:
    io::io_context::strand executor_;
    spdlog::logger& logger_;
    DiceRollsDetails rolls_results_;
    StatsManager stats_;
    std::map<byte, Player> players_;
    std::map<byte, tcp::socket> connections_;
    Game game_;
    std::optional<byte> leader_;
    word current_scene_;
    std::atomic_bool running_;

    const GameBuilder& game_builer_;

    void begin(Entrants& initial_entrants_data);
    void end(Entrants& initial_entrants_data);

    word newGame();
    word gameFromCheckpoint(const std::string& final_name, const bool missing_entrants);
    EntrantsValidity checkEntrants(const GameState& checkpoint, const bool missing_entrants) const;

    void initPlayer(Player& target);
    void restorePlayer(const byte target_id, const PlayerState& previous_state);

    void globalDiceRolls(Gameplay& interface) const;
    void playersDiceRolls(Gameplay& interface) const;

    Next playScene(Gameplay& interface, const word sceneID);

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

    void start(Entrants& initial_entrants_data, const std::string& final_name = "", const bool missing_entrants = false);
    void stop() { running_ = false; }
    bool running() const { return running_; }
    void reset();

    std::string checkpoint(const std::string& generic_name, const word sceneID) const;

    const Game& game() const { return game_; }

    StatsManager& stats() { return stats_; }
    const StatsManager& stats() const { return stats_; }

    const GameBuilder& gameBuilder() const { return game_builer_; }

    Replies request(const byte targets_id, const Data& request_data, ReplyController controller, const bool first_reply_only, const bool wait_all_replies);
    void sendTo(const byte target, const Data& data);
    void sendToAll(const Data& data);

    void disconnect(const byte target, const bool is_crash = false);

    byte leader() const;
    void switchLeader(const byte new_leader);

    Player& player(const byte id);
    const Player& player(const byte id) const;

    std::vector<byte> ids() const;
    Players players();
    ConstPlayers players() const;
    std::size_t count() const { return players_.size(); }
    bool playersRemaining() const { return !players_.empty(); }
};

template<typename Output>
Output& operator<<(Output& out, const Replies& replies) {
    out << '[';
    for (const auto [id, reply] : replies)
        out << ' ' << std::to_string(id) << "->" << std::to_string(reply);

    out << " ]";
    return out; // operator<< retourne osteam& (ref sur classe mère) et non pas Output&
}

} // namespace Rbo

#endif // SESSION_HPP
