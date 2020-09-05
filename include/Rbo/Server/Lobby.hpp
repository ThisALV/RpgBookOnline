#ifndef LOBBY_HPP
#define LOBBY_HPP

#include <Rbo/Session.hpp>

namespace Rbo::Server {

enum struct YesNoQuestion : byte;
enum struct SessionResult : byte;

using MembersName = std::map<byte, std::string>;
using MembersConnection = std::map<byte, tcp::socket>;

struct MasterDisconnected : std::runtime_error {
    MasterDisconnected(const byte id)
        : std::runtime_error { "Membre maître ID=" + std::to_string(id) + " déconnecté" } {}
};

struct PreparationError : std::runtime_error {
    PreparationError(const ErrCode& err)
        : std::runtime_error { "Préparation impossible : " + err.message() } {}
};

struct Run {
    SessionResult result;
    Participants participants;
    std::vector<byte> expectedIDs;

    Run() = default;
    Run(const SessionResult, Participants, const std::vector<byte>& = {});
};

class Lobby {
private:
    static void logMemberError(spdlog::logger&, const byte, const ErrCode&);
    static void logRegisteringError(spdlog::logger&, const tcp::endpoint, const ErrCode&);

    struct RemoteEndpointHash {
        std::size_t operator()(const tcp::endpoint&) const;
    };

    template<typename ValueT>
    using RemoteEndptHashmap = std::unordered_map<tcp::endpoint, ValueT, RemoteEndpointHash>;

    enum LobbyState {
        Open, Starting, Preparing, Running, Closed
    };

    spdlog::logger& logger_;
    io::io_context& lobby_io_;
    io::io_context::strand member_handling_;
    tcp::acceptor new_players_acceptor_;
    tcp::endpoint acceptor_endpt_;

    MembersName players_;
    MembersConnection connections_;
    RemoteEndptHashmap<tcp::socket> registering_;
    RemoteEndptHashmap<ReceiveBuffer> registering_buffers_;
    std::vector<byte> ready_members_;
    std::map<byte, ReceiveBuffer> request_buffers_;

    std::chrono::milliseconds prepare_delay_;
    std::mutex close_;
    std::mutex request_cancelling_;
    std::atomic<LobbyState> state_;
    io::steady_timer prepare_timer_;
    Session session_;
    byte master_;

    void disconnect(const byte, const bool = false);
    void sendToAll(const Data&);
    void sendToAllMasterHandling(const Data&);
    ReceiveBuffer receiveFromMaster();
    void sendToMaster(const Data&);

    void acceptMember();
    void listenMember(const byte);
    void registerMember(const boost::system::error_code, tcp::socket);
    void handleMemberRequest(const byte, const boost::system::error_code, const std::size_t);
    void launchPreparation();
    void makeSession(std::optional<std::string> = {}, std::optional<bool> = {});
    Run runSession(const std::string&, const bool = false);

    void preparation();
    void cancelPreparation(const bool = false);
    std::string askCheckpoint();
    bool askYesNo(const YesNoQuestion);

public:
    Lobby(io::io_context&, const tcp::endpoint&, const GameBuilder&, const ulong = 5);

    Lobby(const Lobby&) = delete;
    Lobby& operator=(const Lobby&) = delete;

    bool operator==(const Lobby&) const = delete;

    void open();
    void close(const bool = false);

    ushort port() const { return acceptor_endpt_.port(); }
    bool registered(const byte id) const { return names().count(id) == 1; }
    bool registered(const std::string&) const;
    bool allMembersReady() const { return readyMembers().size() == names().size(); }

    std::vector<byte> ids() const;
    const MembersName& names() const { return players_; }
    const MembersConnection& connections() const { return connections_; }
    const std::vector<byte>& readyMembers() const { return ready_members_; }

    bool isOpen() const { return state_ == Open; }
    bool isStarting() const { return state_ == Starting; }
    bool isPreparing() const { return state_ == Preparing; }
    bool isRunningSession() const { return state_ == Running; }
    bool isClosed() const { return state_ == Closed; }
};

} // namespace Rbo::Server

#endif // LOBBY_HPP
