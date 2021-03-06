#include <Rbo/Server/Lobby.hpp>

#include <spdlog/logger.h>
#include <spdlog/fmt/ostr.h>
#include <Rbo/GameBuilder.hpp>
#include <Rbo/Server/LobbyDataFactory.hpp>

namespace Rbo::Server {

Run::Run(const SessionResult r, Entrants ps, const std::vector<byte>& expected_ids) : result { r }, entrants { std::move(ps) }, expectedIDs { expected_ids } {}

std::size_t Lobby::RemoteEndpointHash::operator()(const tcp::endpoint& client) const {
    std::ostringstream str;
    str << client;

    return std::hash<std::string> {} (str.str());
}

Lobby::Lobby(io::io_context& lobby_io, tcp::endpoint acceptor_endpt, const ulong preparation_countdown_ms)
    : prepare_delay_ {preparation_countdown_ms },
      acceptor_endpt_ { std::move(acceptor_endpt) },
      logger_ { rboLogger("Lobby") },
      new_players_acceptor_ { lobby_io },
      closure_requested_ { false },
      state_ { Idle },
      prepare_timer_ { lobby_io } {}

void Lobby::logMemberError(const byte id, const ErrCode& err) {
    logger_.error("Member {} : {}", id, err.message());
}

void Lobby::logRegisteringError(const tcp::endpoint& client, const ErrCode& err) {
    logger_.error("Registration of {} : {}", client, err.message());
}

void Lobby::disconnect(const byte id, const bool crash) {
    tcp::socket& client { connections_.at(id) };
    try {
        logger_.info("Member {} disconnected ({}). Crash = {}", id, client.remote_endpoint(), crash);
    } catch (const boost::system::system_error& err) {
        logger_.info("Member {} disconnected (Unable to get address) : Crash = {}", id, crash);
    }

    if (!crash)
        connections_.at(id).shutdown(tcp::socket::shutdown_both);

    members_.erase(id);
    connections_.erase(id);

    if (!isClosed())
        updateMaster();

    const bool empty_lobby { members().empty() };
    if (empty_lobby && isStarting())
        cancelCountdown();
    else if (!empty_lobby && isOpen() && allMembersReady())
        beginCountdown();

    LobbyDataFactory disconnect_data;
    if (crash)
        disconnect_data.makeCrash(id);
    else
        disconnect_data.makeDisconnect(id);

    sendToAll(disconnect_data.dataWithLength());
}

void Lobby::sendToAll(const Data& data) {
    const io::const_buffer buffer { trunc(data) };
    for (const byte id : ids()) {
        const ErrCode send_err { trySend(connections_.at(id), buffer) };

        if (send_err) {
            logger_.error("Sending failed for {} : {}", id, send_err.message());
            disconnect(id, true);
        }
    }
}

bool Lobby::registered(const std::string& name) const {
    return std::any_of(members().cbegin(), members().cend(), [&name](const auto& m) { return m.second.name == name; });
}

std::vector<byte> Lobby::ids() const {
    std::vector<byte> ids;
    ids.resize(members().size());

    std::transform(members().cbegin(), members().cend(), ids.begin(), [](const auto& member) -> byte {
        return member.first;
    });

    return ids;
}

const std::string& Lobby::name(const byte id) const {
    return members().at(id).name;
}

bool Lobby::ready(const byte id) const {
    return members().at(id).ready;
}

void Lobby::beginCountdown() {
    logger_.info("Session preparation into {} ms...", prepare_delay_.count());

    state_ = Starting;
    prepare_timer_.expires_after(prepare_delay_);
    prepare_timer_.async_wait([this](const ErrCode err) {
        if (err) {
            if (!(err == io::error::basic_errors::operation_aborted && isOpen())) {
                cancelCountdown(true);
                logger_.error("Unexpected preparation cancellation : {}", err.message());
            }

            return;
        }

        state_ = Preparing;

        new_players_acceptor_.close();
        for (auto& connection : connections_)
            connection.second.cancel();
    });

    LobbyDataFactory preparation_data;
    preparation_data.makePreparing(prepare_delay_.count());

    sendToAll(preparation_data.dataWithLength());
}

void Lobby::cancelCountdown(const bool is_crash) {
    state_ = Open;
    if (!is_crash)
        prepare_timer_.cancel();

    logger_.info("Preparation cancelled !");

    LobbyDataFactory preparation_data;
    preparation_data.makeEvent(Event::CancelCountdown);

    sendToAll(preparation_data.dataWithLength());
}

void Lobby::open() {
    logger_.info("Opening on port {}.", port());
    state_ = Open;

    new_players_acceptor_.open(acceptor_endpt_.protocol());
    new_players_acceptor_.set_option(tcp::acceptor::reuse_address { true });
    new_players_acceptor_.bind(acceptor_endpt_);
    new_players_acceptor_.listen();

    LobbyDataFactory open_data;
    open_data.makeEvent(Event::Open);

    sendToAll(open_data.dataWithLength());

    master_.reset();
    updateMaster();

    acceptMember();
    logger_.info("Opened.");

    for (auto& connection : connections_)
        listenMember(connection.first);
}

void Lobby::close(const bool crash) {
    logger_.info("Closing...");
    state_ = Closed;

    if (!crash) {
        for (const byte id : ids())
            disconnect(id);
    }

    logger_.info("Closed.");
}

void Lobby::acceptMember() {
    logger_.debug("Waiting for connection on port {}...", port());
    new_players_acceptor_.async_accept([this](const ErrCode err, tcp::socket client_connection) {
        registerMember(err, std::move(client_connection));
    });
}

void Lobby::listenMember(const byte id) {
    logger_.debug("Listening requests of [{}]...", id);
    connections_.at(id).async_receive(io::buffer(request_buffers_.at(id)), [this, id](const ErrCode err, const std::size_t len) {
        handleMemberRequest(id, err, len);
    });
}

void Lobby::registerMember(const ErrCode accept_err, tcp::socket connection) {
    if (accept_err == io::error::basic_errors::operation_aborted && isPreparing()) {
        logger_.debug("Registration cancelled.");
        return;
    }

    acceptMember();

    const tcp::endpoint client_endpt { connection.remote_endpoint() };
    if (accept_err) {
        logRegisteringError(client_endpt, accept_err);
        return;
    }

    logger_.info("Connection established from {}.", client_endpt);
    registering_.insert({ connection.remote_endpoint(), std::move(connection) });
    registering_buffers_.insert({ client_endpt, ReceiveBuffer {} });
    registering_buffers_.at(client_endpt).fill(0);

    registering_.at(client_endpt).async_receive(io::buffer(registering_buffers_.at(client_endpt)), [this, client_endpt](const ErrCode name_err, const std::size_t) {
        handleRegistrationRequest(client_endpt, name_err);
    });
}

void Lobby::handleRegistrationRequest(const tcp::endpoint& client_endpt, const ErrCode& name_err) {
    if (name_err) {
        logRegisteringError(client_endpt, name_err);
        return;
    }

    const ReceiveBuffer& id_name_buffer {registering_buffers_.at(client_endpt) };
    const byte id { id_name_buffer.at(0) };

    std::string name;
    for (std::size_t pos { 1 }; id_name_buffer[pos] != 0; pos++)
        name += id_name_buffer[pos];

    RegistrationResult registration { RegistrationResult::Ok };
    if (isStarting())
        registration = RegistrationResult::UnavailableSession;
    else if (name.empty())
        registration = RegistrationResult::InvalidRequest;
    else if (id == ACTIVE_PLAYERS || id == ALL_PLAYERS)
        registration = RegistrationResult::ReservedID;
    else if (registered(id))
        registration = RegistrationResult::UnavailableID;
    else if (registered(name))
        registration = RegistrationResult::UnavailableName;

    LobbyDataFactory registration_data;
    if (registration == RegistrationResult::Ok)
        registration_data.makeRegistered(members());
    else
        registration_data.makeRegistration(registration);

    const ErrCode send_register_err { trySend(registering_.at(client_endpt), trunc(registration_data.dataWithLength())) };

    if (send_register_err) {
        logRegisteringError(client_endpt, send_register_err);
        return;
    }

    if (registration != RegistrationResult::Ok) {
        logger_.warn("Unable to register [{}] : #{}", client_endpt, static_cast<int>(registration));
        registering_.at(client_endpt).shutdown(tcp::socket::shutdown_both);
        registering_.erase(client_endpt);

        return;
    }

    logger_.info("Successful registration for \"{}\" [{}] ({}).", name, id, client_endpt);
    LobbyDataFactory new_player_data;
    new_player_data.makeNewMember(id, name);

    sendToAll(new_player_data.dataWithLength());

    members_.insert({id, Member {name, false } });
    connections_.insert({id, std::move(registering_.at(client_endpt)) });

    request_buffers_.insert({id, ReceiveBuffer {} });
    request_buffers_.at(id).fill(0);

    registering_.erase(client_endpt);
    registering_buffers_.erase(client_endpt);

    if (!updateMaster())
        sendMaster();

    listenMember(id);
}

enum struct MemberRequest : byte {
    Ready, Disconnect
};

void Lobby::handleMemberRequest(const byte id, const ErrCode request_err, const std::size_t request_len) {
    if (request_err == io::error::basic_errors::operation_aborted && isPreparing()) {
        logger_.debug("Listening to requests canceled for [{}].", id);
        return;
    }

    if (request_err) {
        disconnect(id, true);
        logMemberError(id, request_err);
        return;
    }

    ReceiveBuffer& request_buffer { request_buffers_.at(id) };
    if (request_len != 1 || request_buffer[0] > static_cast<byte>(MemberRequest::Disconnect)) {
        disconnect(id, true);
        logger_.error("Member {} : Invalid request.", id);
        return;
    }

    switch (static_cast<MemberRequest>(request_buffer[0])) {
    case MemberRequest::Ready: {
        if (ready(id)) {
            logger_.info("Member \"{}\" [{}] no longer ready.", name(id), id);
            members_.at(id).ready = false;
        } else {
            logger_.info("Member \"{}\" [{}] ready.", name(id), id);
            members_.at(id).ready = true;
        }

        std::vector<byte> ready_members;
        for (const auto& [id, member] : members()) {
            if (member.ready)
                ready_members.push_back(id);
        }

        logger_.debug("Ready members : {}", ByteVecWrapper { ready_members });

        LobbyDataFactory ready_data;
        ready_data.makeReady(id);

        sendToAll(ready_data.dataWithLength());

        if (!isStarting() && allMembersReady() && !members().empty())
            beginCountdown();
        else if (isStarting() && !allMembersReady())
            cancelCountdown();

        break;
    } case MemberRequest::Disconnect:
        disconnect(id);
        return;
    }

    request_buffer.fill(0);
    listenMember(id);
}

void Lobby::safeSendToAll(const Data& data) {
    const bool was_here { master_ };
    const std::optional<byte> prev_master { master_ };

    sendToAll(data);

    if (was_here && (!master_ || *prev_master != master_ ))
        throw MasterDisconnected { *prev_master };
}

ReceiveBuffer Lobby::receiveFromMaster() {
    ReceiveBuffer buffer;
    buffer.fill(0);

    std::atomic_bool received { false };
    bool receive_err { false };

    connections_.at(*master_).async_receive(io::buffer(buffer), [&received, &receive_err](const ErrCode err, const std::size_t) {
        received = true;

        if (err)
            receive_err = true;
    });

    while (!received && !closure_requested_)
        std::this_thread::sleep_for(std::chrono::milliseconds { 1 });

    if (receive_err)
        disconnectMaster();

    return buffer;
}

void Lobby::sendToMaster(const Data& data) {
    const ErrCode send_err { trySend(connections_.at(*master_), trunc(data)) };

    if (send_err)
        disconnectMaster();
}

bool Lobby::updateMaster() {
    bool updated;
    if (members().empty()) {
        updated = master_.has_value();
        master_.reset();

        if (updated)
            logger_.info("Not any master member, lobby is empty.");
    } else {
        const std::optional<byte> prev_master { master_ };
        master_ = members().cbegin()->first;

        updated = !prev_master || *prev_master != master_;
        if (updated)
            logger_.info("New master member is [{}].", *master_);
    }

    if (updated)
        sendMaster();

    return updated;
}

void Lobby::sendMaster() {
    LobbyDataFactory master_data;
    master_data.makeMasterSwitch(master_);

    sendToAll(master_data.dataWithLength());
}

void Lobby::disconnectMaster() {
    assert(master_.has_value());

    disconnect(*master_, true);

    throw MasterDisconnected { *master_ };
}

std::string Lobby::askCheckpoint() {
    LobbyDataFactory ask_data;
    ask_data.makeEvent(Event::AskCheckpoint);

    logger_.info("Asking master [{}] for checkpoint...", *master_);
    sendToMaster(ask_data.dataWithLength());

    const ReceiveBuffer chkpt_buffer { receiveFromMaster() };

    std::string chkpt_name;
    if (closure_requested_) {
        logger_.info("Closure requested, master's reply ignored.");
    } else if (chkpt_buffer[0] != 0) {
        for (std::size_t pos { 0 }; chkpt_buffer[pos] != 0; pos++)
            chkpt_name += chkpt_buffer[pos];

        logger_.info("Checkpoint : \"{}\"", chkpt_name);
    }

    return chkpt_name;
}

bool Lobby::askYesNo(const YesNoQuestion request) {
    LobbyDataFactory ask_data;
    ask_data.makeYesNo(request);

    logger_.info("Asking master [{}] for #{}...", *master_, static_cast<int>(request));
    sendToMaster(ask_data.dataWithLength());

    const ReceiveBuffer reply { receiveFromMaster() };

    if (closure_requested_)
        logger_.info("Closure requested, master's reply ignored.");
    else
        logger_.info("Master's reply : {}.", reply[0] == 0 ? "Yes" : "No");

    return reply[0] == YES;
}

void Lobby::reset() {
    logger_.info("Resetting state to idle...");
    state_ = Idle;

    for (auto& member : members_)
        member.second.ready = false;

    logger_.info("Reset.");
}

void Lobby::prepareSession(Session& session) {
    try {
        LobbyDataFactory prepare_data;
        prepare_data.makePrepare(*master_);

        logger_.info("Preparing session, master member is {}.", *master_);
        safeSendToAll(prepare_data.dataWithLength());

        configureSession(session);
    } catch (const MasterDisconnected& err) {
        logger_.error(err.what());

        LobbyDataFactory master_dc_data;
        master_dc_data.makeEvent(Event::MasterDisconnected);

        sendToAll(master_dc_data.dataWithLength());

        for (auto& member : members_)
            member.second.ready = false;
    } catch (const NoPlayerRemaining& err) {
        logger_.error(err.what());
    }

    state_ = Idle;
}

void Lobby::configureSession(Session& session, const std::optional<std::string>& chkpt_name, std::optional<bool> missing_entrants) {
    assert(!(missing_entrants && chkpt_name->empty()));

    std::string requested_chkpt;
    if (chkpt_name) {
        requested_chkpt = *chkpt_name;
    } else {
        LobbyDataFactory select_chkpt_data;
        select_chkpt_data.makeEvent(Event::SelectingCheckpoint);

        safeSendToAll(select_chkpt_data.dataWithLength());

        requested_chkpt = askCheckpoint();
    }

    if (!missing_entrants) {
        LobbyDataFactory checking_players_data;
        checking_players_data.makeEvent(Event::CheckingPlayers);

        safeSendToAll(checking_players_data.dataWithLength());

        missing_entrants = !requested_chkpt.empty() && askYesNo(YesNoQuestion::MissingEntrants);
    }

    if (closure_requested_) {
        logger_.info("Closure requested, session canceled.");
        return;
    }

    Run run { runSession(session, requested_chkpt, *missing_entrants) };
    members_.clear();
    connections_.clear();

    logger_.trace("Sending session's result to entrants...");

    for (auto& [id, entrant] : run.entrants) {
        members_.insert({ id, Member {entrant.name, false } });
        connections_.insert({ id, std::move(entrant.socket) });
    }

    LobbyDataFactory run_data;
    if (isInvalidIDs(run.result))
        run_data.makeInvalidIDs(run.result, run.expectedIDs);
    else
        run_data.makeResult(run.result);

    safeSendToAll(run_data.dataWithLength());

    if (isParametersError(run.result)) {
        sendMaster();

        LobbyDataFactory revising_session_data;
        revising_session_data.makeEvent(Event::RevisingParameters);

        safeSendToAll(revising_session_data.dataWithLength());
    }

    switch (run.result) {
    case SessionResult::Ok:
    case SessionResult::Crashed:
        return;
    case SessionResult::CheckpointLoadingError:
        if (askYesNo(YesNoQuestion::RetryCheckpoint)) {
            session.reset();
            configureSession(session);
        }

        break;
    case SessionResult::LessMembers:
        if (askYesNo(YesNoQuestion::MissingEntrants)) {
            session.reset();
            configureSession(session, chkpt_name, true);
        } else if (askYesNo(YesNoQuestion::RetryCheckpoint)) {
            session.reset();
            configureSession(session);
        }

        break;
    case SessionResult::UnknownPlayer:
        if (askYesNo(YesNoQuestion::KickUnknownPlayers)) {
            const auto& b { run.expectedIDs.cbegin() };
            const auto& e { run.expectedIDs.cend() };

            if (std::find(b, e, master_) == e) {
                disconnect(*master_);
                throw MasterDisconnected { *master_ };
            }

            for (const byte id : ids()) {
                if (std::find(b, e, id) == e)
                    disconnect(id);
            }

            session.reset();
            configureSession(session, chkpt_name);
        } else if (askYesNo(YesNoQuestion::RetryCheckpoint)) {
            session.reset();
            configureSession(session);
        }

        break;
    case SessionResult::NoPlayerAlive:
        if (askYesNo(YesNoQuestion::RetryCheckpoint)) {
            session.reset();
            configureSession(session);
        }

        break;
    }
}

Run Lobby::runSession(Session& session, const std::string& chkpt_name, const bool missing_entrants) {
    LobbyDataFactory start_data;
    start_data.makeEvent(Event::Start);

    logger_.info("Starting session...");
    logger_.trace("Checkpoint=\"{}\" ; Missing entrants ? {}", chkpt_name, missing_entrants);
    safeSendToAll(start_data.dataWithLength());

    Entrants entrants;
    for (const auto& [id, member] : members())
        entrants.insert({ id, Entrant { member.name, std::move(connections_.at(id)) } });

    try {
        session.start(entrants, chkpt_name, missing_entrants);
        logger_.info("Session end.");
    } catch (const CheckpointLoadingError& err) {
        logger_.error("Unable to load checkpoint \"{}\" : {}", chkpt_name, err.what());
        return {SessionResult::CheckpointLoadingError, std::move(entrants)};
    } catch (const NoEntrantAlive& err) {
        logger_.error("For \"{}\" : {}", chkpt_name, err.what());
        return { SessionResult::NoPlayerAlive, std::move(entrants) };
    } catch (const InvalidIDs& err) {
        Run run;
        run.expectedIDs = err.expectedIDs;
        logger_.error("Expected entrants IDs : {}", ByteVecWrapper { err.expectedIDs });

        if (err.errType == EntrantsValidity::UnknownPlayer) {
            logger_.error("Unknown IDs among us.");
            return { SessionResult::UnknownPlayer, std::move(entrants), err.expectedIDs };
        } else {
            logger_.error("Less entrants then expected.");
            return { SessionResult::LessMembers, std::move(entrants), err.expectedIDs };
        }
    } catch (const std::exception& err) {
        logger_.error("Session crashed : {}", err.what());
        return { SessionResult::Crashed, std::move(entrants) };
    }

    return { SessionResult::Ok, std::move(entrants) };
}

} // namespace Rbo::Server
