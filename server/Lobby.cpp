#include <Rbo/Server/Lobby.hpp>

#include <spdlog/logger.h>
#include <spdlog/fmt/ostr.h>
#include <Rbo/GameBuilder.hpp>
#include <Rbo/Server/LobbyDataFactory.hpp>

namespace Rbo::Server {

void Lobby::logMemberError(spdlog::logger& logger, const byte id, const ErrCode& err) {
    logger.error("Member {} : {}", id, err.message());
}

void Lobby::logRegisteringError(spdlog::logger& logger, const tcp::endpoint client, const ErrCode& err) {
    logger.error("Registration of {} : {}", client, err.message());
}

Run::Run(const SessionResult r, Participants ps, const std::vector<byte>& expected_ids) : result { r }, participants { std::move(ps) }, expectedIDs { expected_ids } {}

std::size_t Lobby::RemoteEndpointHash::operator()(const tcp::endpoint& client) const {
    std::ostringstream str;
    str << client;

    return std::hash<std::string> {} (str.str());
}

Lobby::Lobby(io::io_context& lobby_io, const tcp::endpoint& acceptor_endpt, const GameBuilder& game_builder, const ulong prepare_delay_ms)
    : logger_ { rboLogger("Lobby") },
      lobby_io_ { lobby_io },
      member_handling_ { lobby_io },
      new_players_acceptor_ { lobby_io },
      acceptor_endpt_ { acceptor_endpt },
      prepare_delay_ { prepare_delay_ms },
      state_ { Closed },
      prepare_timer_ { lobby_io },
      session_ { lobby_io, game_builder } {}

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

    if (members().empty() && isStarting())
        cancelPreparation();
    else if (!members().empty() && isOpen() && allMembersReady())
        preparation();

    LobbyDataFactory disconnect_data;
    if (crash)
        disconnect_data.makeCrash(id);
    else
        disconnect_data.makeDisconnect(id);

    sendToAll(disconnect_data.dataWithLength());
}

void Lobby::sendToAll(const Data& data) {
    const io::const_buffer buffer { trunc(data) };
    for (auto& [id, connection] : connections_) {
        const ErrCode send_err { trySend(connection, buffer) };

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

void Lobby::preparation() {
    logger_.info("Session preparation into {} ms...", prepare_delay_.count());

    state_ = Starting;
    prepare_timer_.expires_after(prepare_delay_);
    prepare_timer_.async_wait([this](const ErrCode err) {
        if (err) {
            if (!(err == io::error::basic_errors::operation_aborted && isOpen())) {
                cancelPreparation(true);
                logger_.error("Unexpected preparation cancellation : {}", err.message());
            }

            return;
        }

        launchPreparation();
    });

    LobbyDataFactory preparation_data;
    preparation_data.makePreparing(prepare_delay_.count());

    sendToAll(preparation_data.dataWithLength());
}

void Lobby::cancelPreparation(const bool crash) {
    state_ = Open;
    if (!crash)
        prepare_timer_.cancel();

    logger_.info("Preparation cancelled !");

    LobbyDataFactory preparation_data;
    preparation_data.makeState(State::CancelPreparing);

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
    open_data.makeState(State::Open);

    sendToAll(open_data.dataWithLength());
    acceptMember();
    logger_.info("Opened.");

    for (auto& connection : connections_)
        listenMember(connection.first);
}

void Lobby::close(const bool crash) {
    logger_.info("Closing...");
    session_.stop();

    state_ = Closed;
    const std::lock_guard close_guard { close_ };
    logger_.info("Closed.");

    if (!crash) {
        for (const byte id : ids())
            disconnect(id);
    }
}

void Lobby::acceptMember() {
    logger_.debug("Waiting for connection on port {}...", port());
    new_players_acceptor_.async_accept(
                io::bind_executor(member_handling_, std::bind(&Lobby::registerMember, this, std::placeholders::_1, std::placeholders::_2)));
}

void Lobby::listenMember(const byte id) {
    const std::lock_guard cancelling_lock { request_cancelling_ };
    if (isPreparing())
        return;

    logger_.debug("Listening requests of [{}]...", id);
    connections_.at(id).async_receive(io::buffer(request_buffers_.at(id)),
                                      io::bind_executor(member_handling_, std::bind(&Lobby::handleMemberRequest, this, id, std::placeholders::_1, std::placeholders::_2)));
}

void Lobby::registerMember(const ErrCode accept_err, tcp::socket connection) {
    if (accept_err == io::error::basic_errors::operation_aborted && isPreparing()) {
        logger_.debug("Registration cancelled.");
        return;
    }

    const std::lock_guard close_guard { close_ };
    if (isClosed())
        return;

    acceptMember();

    const tcp::endpoint client_endpt { connection.remote_endpoint() };
    if (accept_err) {
        logRegisteringError(logger_, client_endpt, accept_err);
        return;
    }

    logger_.info("Connection established from {}.", client_endpt);
    registering_.insert({ connection.remote_endpoint(), std::move(connection) });
    registering_buffers_.insert({ client_endpt, ReceiveBuffer {} });
    registering_buffers_.at(client_endpt).fill(0);

    registering_.at(client_endpt).async_receive(io::buffer(registering_buffers_.at(client_endpt)), io::bind_executor(member_handling_, [this, client_endpt](const ErrCode name_err, const std::size_t) mutable {
        if (name_err) {
            logRegisteringError(logger_, client_endpt, name_err);
            return;
        }

        const ReceiveBuffer& id_name_buffer { registering_buffers_.at(client_endpt) };
        const byte id { id_name_buffer.at(0) };

        std::string name;
        for (std::size_t pos { 1 }; id_name_buffer[pos] != 0; pos++)
            name += id_name_buffer[pos];

        RegistrationResult registration { RegistrationResult::Ok };
        if (isStarting())
            registration = RegistrationResult::UnavailableSession;
        else if (name.empty())
            registration = RegistrationResult::InvalidRequest;
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
            logRegisteringError(logger_, client_endpt, send_register_err);
            return;
        }

        if (registration != RegistrationResult::Ok) {
            logger_.warn("Unable to register [{}] : {}", client_endpt, static_cast<int>(registration));
            registering_.at(client_endpt).shutdown(tcp::socket::shutdown_both);
            registering_.erase(client_endpt);

            return;
        }

        logger_.info("Successful registration for \"{}\" [{}] ({}).", name, id, client_endpt);
        LobbyDataFactory new_player_data;
        new_player_data.makeNewMember(id, name);

        sendToAll(new_player_data.dataWithLength());

        members_.insert({ id, Member { name, false } });
        connections_.insert({ id, std::move(registering_.at(client_endpt)) });

        request_buffers_.insert({ id, ReceiveBuffer {} });
        request_buffers_.at(id).fill(0);

        registering_.erase(client_endpt);
        registering_buffers_.erase(client_endpt);

        listenMember(id);
    }));
}

enum struct MemberRequest : byte {
    Ready, Disconnect
};

void Lobby::handleMemberRequest(const byte id, const ErrCode request_err, const std::size_t) {
    if (request_err == io::error::basic_errors::operation_aborted && isPreparing()) {
        logger_.debug("Listening to requests canceled for [{}].", id);
        return;
    }

    const std::lock_guard close_guard { close_ };
    if (isClosed())
        return;

    if (request_err) {
        disconnect(id, true);
        logMemberError(logger_, id, request_err);
        return;
    }

    ReceiveBuffer& request_buffer { request_buffers_.at(id) };
    if (std::any_of(request_buffer.cbegin() + 1, request_buffer.cend(), [](const byte b) { return b != 0; }) || request_buffer[0] > 1) {
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

        logger_.debug("Ready members : {}", ready_members);

        LobbyDataFactory ready_data;
        ready_data.makeReady(id);

        sendToAll(ready_data.dataWithLength());

        if (!isStarting() && allMembersReady() && !members().empty())
            preparation();
        else if (isStarting() && !allMembersReady())
            cancelPreparation();

        break;
    } case MemberRequest::Disconnect:
        disconnect(id);
        return;
    }

    request_buffer.fill(0);
    listenMember(id);
}

void Lobby::sendToAllMasterHandling(const Data& data) {
    const bool was_here { registered(master_) };

    sendToAll(data);

    if (was_here && !registered(master_))
        throw MasterDisconnected { master_ };
}

ReceiveBuffer Lobby::receiveFromMaster() {
    ReceiveBuffer buffer;
    buffer.fill(0);

    std::atomic_bool received { false };
    bool receive_err { false };

    connections_.at(master_).async_receive(io::buffer(buffer), [this, &received, &receive_err](const ErrCode err, const std::size_t) {
        received = true;
        if (err && (err != io::error::basic_errors::operation_aborted || !isClosed()))
            receive_err = true;
    });

    while (!received && !isClosed())
        std::this_thread::sleep_for(std::chrono::milliseconds { 1 });

    if (isClosed())
        connections_.at(master_).cancel();

    if (receive_err) {
        disconnect(master_, true);
        throw MasterDisconnected { master_ };
    }

    return buffer;
}

void Lobby::sendToMaster(const Data& data) {
    const ErrCode send_err { trySend(connections_.at(master_), trunc(data)) };
    if (send_err) {
        disconnect(master_, true);
        throw MasterDisconnected { master_ };
    }
}

std::string Lobby::askCheckpoint() {
    LobbyDataFactory ask_data;
    ask_data.makeState(State::AskCheckpoint);

    logger_.info("Asking master [{}] for checkpoint...", master_);
    sendToMaster(ask_data.dataWithLength());

    const ReceiveBuffer chkpt_buffer { receiveFromMaster() };
    std::string chkpt_name;

    if (chkpt_buffer[0] != 0) {
        for (std::size_t pos { 0 }; chkpt_buffer[pos] != 0; pos++)
            chkpt_name += chkpt_buffer[pos];
    }

    logger_.info("Checkpoint : \"{}\"", chkpt_name);
    return chkpt_name;
}

bool Lobby::askYesNo(const YesNoQuestion request) {
    LobbyDataFactory ask_data;
    ask_data.makeYesNo(request);

    logger_.info("Asking master [{}] for #{}...", master_, static_cast<int>(request));
    sendToMaster(ask_data.dataWithLength());

    const ReceiveBuffer reply { receiveFromMaster() };
    logger_.info("Master's reply : {}.", reply[0] == 0 ? "Yes" : "No");

    return reply[0] == 0;
}

void Lobby::launchPreparation() {
    state_ = Preparing;
    new_players_acceptor_.close();

    std::unique_lock cancelling_lock { request_cancelling_ };
    for (auto& connection : connections_)
        connection.second.cancel();
    cancelling_lock.unlock();

    try {
        master_ = members().cbegin()->first;

        LobbyDataFactory prepare_data;
        prepare_data.makePrepare(master_);

        logger_.info("Preparing session, master member is {}.", master_);
        sendToAllMasterHandling(prepare_data.dataWithLength());

        const std::lock_guard close_guard { close_ };
        makeSession();
    } catch (const MasterDisconnected& err) {
        logger_.error(err.what());

        LobbyDataFactory master_dc_data;
        master_dc_data.makeState(State::MasterDisconnected);

        sendToAll(master_dc_data.dataWithLength());
    } catch (const NoPlayerRemaining& err) {
        logger_.error(err.what());
    }

    if (!isClosed())
        open();
}

void Lobby::makeSession(std::optional<std::string> chkpt_name, std::optional<bool> missing_participants) {
    assert(!(missing_participants && chkpt_name->empty()));
    if (!chkpt_name) {
        LobbyDataFactory select_chkpt_data;
        select_chkpt_data.makeState(State::SelectingCheckpoint);

        sendToAllMasterHandling(select_chkpt_data.dataWithLength());

        chkpt_name = askCheckpoint();
    }

    if (!missing_participants) {
        LobbyDataFactory checking_players_data;
        checking_players_data.makeState(State::CheckingPlayers);

        sendToAllMasterHandling(checking_players_data.dataWithLength());

        missing_participants = !chkpt_name->empty() && askYesNo(YesNoQuestion::MissingParticipants);
    }

    Run run { runSession(*chkpt_name, *missing_participants) };
    session_.reset();
    members_.clear();
    connections_.clear();

    if (run.result != SessionResult::Crashed) {
        logger_.trace("Sending session's result to participants...");

        for (auto& [id, participant] : run.participants) {
            members_.insert({ id, Member { participant.name, false } });
            connections_.insert({ id, std::move(participant.socket) });
        }

        LobbyDataFactory run_data;
        if (isInvalidIDs(run.result))
            run_data.makeInvalidIDs(run.result, run.expectedIDs);
        else
            run_data.makeResult(run.result);

        sendToAllMasterHandling(run_data.dataWithLength());
    }

    if (!isParametersError(run.result))
        return;

    if (run.result == SessionResult::CheckpointLoadingError) {
        makeSession();
    } else {
        const bool retry_chkpt { askYesNo(YesNoQuestion::RetryCheckpoint) };
        const std::string new_ckpt = retry_chkpt ? *chkpt_name : askCheckpoint();

        if (!retry_chkpt) {
            makeSession(new_ckpt);
        } else if (run.result == SessionResult::LessMembers && askYesNo(YesNoQuestion::MissingParticipants)) {
            makeSession(new_ckpt, true);
        } else if (run.result == SessionResult::UnknownPlayer && askYesNo(YesNoQuestion::KickUnknownPlayers)) {
            const auto cb { run.expectedIDs.cbegin() };
            const auto ce { run.expectedIDs.cend() };


            for (const byte id : ids()) {
                if (std::find(cb, ce, id) == ce)
                    disconnect(id);
            }

            if (!registered(master_))
                throw MasterDisconnected { master_ };

            makeSession(new_ckpt);
        }
    }
}

Run Lobby::runSession(const std::string& chkpt_name, const bool missing_participants) {
    LobbyDataFactory start_data;
    start_data.makeState(State::Start);

    logger_.info("Starting session...");
    logger_.trace("Checkpoint=\"{}\" ; Missing participants ? {}", chkpt_name, missing_participants);
    sendToAllMasterHandling(start_data.dataWithLength());

    Participants participants;
    for (const auto& [id, member] : members())
        participants.insert({ id, Particpant { member.name, std::move(connections_.at(id)) } });

    try {
        if (isClosed())
            return { SessionResult::Ok, std::move(participants) };

        session_.start(participants, chkpt_name, missing_participants);
        logger_.info("Session end.");
    } catch (const CheckpointLoadingError& err) {
        logger_.error("Unable to load checkpoint \"{}\" : {}", chkpt_name, err.what());
        return { SessionResult::CheckpointLoadingError, std::move(participants) };
    } catch (const InvalidIDs& err) {
        Run run;
        run.expectedIDs = err.expectedIDs;
        logger_.error("Expected participants IDs : {}", err.expectedIDs);

        if (err.errType == ParticipantsValidity::UnknownPlayer) {
            logger_.error("Unknown IDs among us.");
            return { SessionResult::UnknownPlayer, std::move(participants), err.expectedIDs };
        } else {
            logger_.error("Less participants then expected.");
            return { SessionResult::LessMembers, std::move(participants), err.expectedIDs };
        }
    } catch (const std::exception& err) {
        logger_.error("Session crashed : {}", err.what());
        return { SessionResult::Crashed, std::move(participants) };
    }

    return { SessionResult::Ok, std::move(participants) };
}

} // namespace Rbo::Server
