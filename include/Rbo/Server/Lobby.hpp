#ifndef LOBBY_HPP
#define LOBBY_HPP

#include <Rbo/Server/ServerCommon.hpp>

#include <Rbo/Session.hpp>

namespace Rbo::Server {

enum struct YesNoQuestion : byte;
enum struct SessionResult : byte;

using MembersConnection = std::map<byte, tcp::socket>;

struct MasterDisconnected : std::runtime_error {
    explicit MasterDisconnected(const byte id) : std::runtime_error { "Master [" + std::to_string(id) + "] disconnected" } {}
};

struct Run {
    SessionResult result;
    Entrants entrants;
    std::vector<byte> expectedIDs;

    Run() = default;
    Run(const SessionResult, Entrants, const std::vector<byte>& = {});
};

class Lobby {
private:
    struct RemoteEndpointHash {
        std::size_t operator()(const tcp::endpoint& client) const;
    };

    template<typename ValueT>
    using RemoteEndptHashmap = std::unordered_map<tcp::endpoint, ValueT, RemoteEndpointHash>;

    enum LobbyState {
        Open, Starting, Preparing, Running, Closed
    };

    const std::chrono::milliseconds prepare_delay_;
    const tcp::endpoint acceptor_endpt_;

    spdlog::logger& logger_;
    io::io_context& lobby_io_;
    io::io_context::strand member_handling_;
    tcp::acceptor new_players_acceptor_;

    MembersStates members_;
    MembersConnection connections_;
    RemoteEndptHashmap<tcp::socket> registering_;
    RemoteEndptHashmap<ReceiveBuffer> registering_buffers_;

    std::map<byte, ReceiveBuffer> request_buffers_;
    std::mutex close_;
    std::mutex request_cancelling_;
    std::atomic<LobbyState> state_;
    io::steady_timer prepare_timer_;
    Session session_;
    Master master_;

    void disconnect(const byte member_id, const bool is_crash = false);
    void sendToAll(const Data& data);
    void safeSendToAll(const Data& data);
    ReceiveBuffer receiveFromMaster();
    void sendToMaster(const Data& data);
    void acceptMember();

    void listenMember(const byte id);
    void registerMember(const ErrCode err, tcp::socket connection);
    void handleRegistrationRequest(const tcp::endpoint& client_endpt, const ErrCode& name_err);
    void handleMemberRequest(const byte member_id, const ErrCode err, const std::size_t request_len);
    void updateMaster();
    void disconnectMaster();
    void launchPreparation();
    void makeSession(const std::optional<std::string>& checkpt_final_name = {}, std::optional<bool> missing_entrants = {});
    Run runSession(const std::string& checkpt_final_name, const bool missing_entrants = false);

    void beginCountdown();
    void cancelCountdown(const bool is_crash = false);
    std::string askCheckpoint();
    bool askYesNo(const YesNoQuestion question);

    void logMemberError(const byte member_id, const ErrCode& err);
    void logRegisteringError(const tcp::endpoint& client, const ErrCode& err);
public:
    Lobby(io::io_context& lobby_io, tcp::endpoint acceptor_endpt, const GameBuilder& game_builder, const ulong prepare_delay_ms = 5000);

    Lobby(const Lobby&) = delete;
    Lobby& operator=(const Lobby&) = delete;

    bool operator==(const Lobby&) const = delete;

    void open();
    void close(const bool is_crash = false);

    ushort port() const { return acceptor_endpt_.port(); }
    bool registered(const byte id) const { return members().count(id) == 1; }
    bool registered(const std::string& name) const;
    bool allMembersReady() const {
        return std::all_of(members().cbegin(), members().cend(), [](const auto& m) { return m.second.ready; });
    }

    std::vector<byte> ids() const;
    const MembersStates& members() const { return members_; }
    const MembersConnection& connections() const { return connections_; }
    const std::string& name(const byte id) const;
    bool ready(const byte id) const;

    bool isOpen() const { return state_ == Open; }
    bool isStarting() const { return state_ == Starting; }
    bool isPreparing() const { return state_ == Preparing; }
    bool isRunningSession() const { return state_ == Running; }
    bool isClosed() const { return state_ == Closed; }
};

} // namespace Rbo::Server

#endif // LOBBY_HPP
