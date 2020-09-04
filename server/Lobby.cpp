#include <Rbo/Server/Lobby.hpp>

#include <spdlog/logger.h>
#include <spdlog/fmt/ostr.h>
#include <Rbo/GameBuilder.hpp>
#include <Rbo/Server/LobbyDataFactory.hpp>

namespace Rbo::Server {

void Lobby::logMemberError(spdlog::logger& logger, const byte id, const ErrCode& err) {
    logger.error("Membre {} : {}", id, err.message());
}

void Lobby::logRegisteringError(spdlog::logger& logger, const tcp::endpoint client,
                                const ErrCode& err)
{
    logger.error("Inscription de {} : {}", client, err.message());
}

Run::Run(const SessionResult r, Participants ps, const std::vector<byte>& expected_ids)
    : result { r }, participants { std::move(ps) }, expectedIDs { expected_ids } {}

std::size_t Lobby::RemoteEndpointHash::operator()(const tcp::endpoint& client) const {
    std::ostringstream str;
    str << client;

    return std::hash<std::string>{}(str.str());
}

Lobby::Lobby(io::io_context& lobby_io, const tcp::endpoint& acceptor_endpt,
             const GameBuilder& game_builder, const ulong prepare_delay_ms)
    : logger_ { rboLogger("Lobby") },
      lobby_io_ { lobby_io },
      member_handling_ { lobby_io },
      new_players_acceptor_ { lobby_io, acceptor_endpt },
      prepare_delay_ { prepare_delay_ms },
      state_ { Closed },
      prepare_timer_ { lobby_io },
      session_ { lobby_io, game_builder } {}

void Lobby::disconnect(const byte id, const bool crash) {
    logger_.info("Déconnexion du membre {} ({}). Crash = {}", id,
                 connections_.at(id).remote_endpoint(), crash);

    if (!crash)
        connections_.at(id).shutdown(tcp::socket::shutdown_both);

    const auto ready_member { std::find(ready_members_.cbegin(), ready_members_.cend(), id) };
    if (ready_member != ready_members_.cend())
        ready_members_.erase(ready_member);

    players_.erase(id);
    connections_.erase(id);

    if (names().empty() && isStarting())
        cancelPreparation();
    else if (!names().empty() && isOpen() && allMembersReady())
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
            logger_.error("L'envoi a échoué pour le membre {} : {}", id, send_err.message());
            disconnect(id, true);
        }
    }
}

bool Lobby::registered(const std::string& name) const {
    const auto b { names().cbegin() };
    const auto e { names().cend() };

    return std::find_if(b, e, [&name](const auto& p) -> bool { return p.second == name; }) != e;
}

std::vector<byte> Lobby::ids() const {
    std::vector<byte> ids;
    ids.resize(names().size());

    std::transform(names().cbegin(), names().cend(), ids.begin(),
                   [](const auto& member) -> byte { return member.first; });

    return ids;
}

void Lobby::preparation() {
    logger_.info("Préparation de la session dans {} ms...", prepare_delay_.count());

    state_ = Starting;
    prepare_timer_.expires_after(prepare_delay_);
    prepare_timer_.async_wait([this](const ErrCode err)
    {
        if (err) {
            if (!(err == io::error::basic_errors::operation_aborted && isOpen())) {
                cancelPreparation(true);
                logger_.error("Annulation imprévue de la préparation : {}", err.message());
            }

            return;
        }

        launchPreparation();
    });

    LobbyDataFactory preparation_data;
    preparation_data.makeState(State::Preparing);

    sendToAll(preparation_data.dataWithLength());
}

void Lobby::cancelPreparation(const bool crash) {
    logger_.info("Préparation annulée !");

    state_ = Open;
    if (!crash)
        prepare_timer_.cancel();

    LobbyDataFactory preparation_data;
    preparation_data.makeState(State::Preparing);

    sendToAll(preparation_data.dataWithLength());
}

void Lobby::open() {
    logger_.info("Ouverture sur le port {}.", port());
    state_ = Open;

    LobbyDataFactory open_data;
    open_data.makeState(State::Open);

    sendToAll(open_data.dataWithLength());
    acceptMember();

    for (auto& connection : connections_)
        listenMember(connection.first);
}

void Lobby::close(const bool crash) {
    logger_.info("Fermeture.");
    session_.stop();

    state_ = Closed;
    const std::lock_guard close_guard { close_ };

    if (!crash) {
        for (const byte id : ids())
            disconnect(id);
    }
}

void Lobby::acceptMember() {
    logger_.trace("En attente d'une connexion sur le port {}...", port());
    new_players_acceptor_.async_accept(io::bind_executor(member_handling_,
                std::bind(&Lobby::registerMember, this,
                          std::placeholders::_1, std::placeholders::_2)));
}

void Lobby::listenMember(const byte id) {
    const std::lock_guard cancelling_lock { request_cancelling_ };
    if (isPreparing())
        return;

    logger_.trace("Écoute des requêtes de {}...", id);
    connections_.at(id).async_receive(io::buffer(request_buffers_.at(id)),
                                      io::bind_executor(member_handling_, std::bind(
                                              &Lobby::handleMemberRequest, this, id,
                                              std::placeholders::_1, std::placeholders::_2)));
}

void Lobby::registerMember(const ErrCode accept_err, tcp::socket connection) {
    if (accept_err == io::error::basic_errors::operation_aborted && isPreparing()) {
        logger_.trace("Annulation de l'inscription.");
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

    logger_.info("Nouvelle connexion depuis {}.", client_endpt);
    registering_.insert({ connection.remote_endpoint(), std::move(connection) });
    registering_buffers_.insert({ client_endpt, ReceiveBuffer {} });
    registering_buffers_.at(client_endpt).fill(0);

    registering_.at(client_endpt).async_receive(
                io::buffer(registering_buffers_.at(client_endpt)),
                io::bind_executor(member_handling_, [this, client_endpt]
                                  (const ErrCode name_err, const std::size_t) mutable
    {
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
        registration_data.makeRegistration(registration);

        const ErrCode send_register_err {
            trySend(registering_.at(client_endpt), trunc(registration_data.dataWithLength()))
        };

        if (send_register_err) {
            logRegisteringError(logger_, client_endpt, send_register_err);
            return;
        }

        if (registration != RegistrationResult::Ok) {
            logger_.warn("Inscription de {} impossible : {}", client_endpt,
                         static_cast<int>(registration));
            registering_.at(client_endpt).shutdown(tcp::socket::shutdown_both);
            registering_.erase(client_endpt);

            return;
        }

        logger_.info("Inscription de \"{}\" [{}] ({}) réussie.", name, id, client_endpt);
        LobbyDataFactory new_player_data;
        new_player_data.makeNewMember(id, name);

        sendToAll(new_player_data.dataWithLength());

        players_.insert({ id, name });
        connections_.insert({ id, std::move(registering_.at(client_endpt)) });

        request_buffers_.insert({ id, ReceiveBuffer {} });
        request_buffers_.at(id).fill(0);

        registering_.erase(client_endpt);
        registering_buffers_.erase(client_endpt);

        for (const byte player : readyMembers()) {
            LobbyDataFactory ready_data;
            ready_data.makeReady(player);

            const ErrCode send_err {
                trySend(connections_.at(id), trunc(ready_data.dataWithLength()))
            };

            if (send_err) {
                disconnect(id, true);
                logMemberError(logger_, id, send_err);
                return;
            }
        }

        listenMember(id);
    }));
}

enum struct MemberRequest : byte {
    Ready, Disconnect
};

void Lobby::handleMemberRequest(const byte id, const ErrCode request_err, const std::size_t) {
    if (request_err == io::error::basic_errors::operation_aborted && isPreparing()) {
        logger_.trace("Annulation de la gestion des requêtes de {}.", id);
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
        logger_.error("Membre {} : Requête invalide.", id);
        return;
    }

    switch (static_cast<MemberRequest>(request_buffer[0])) {
    case MemberRequest::Ready: {
        const auto b { ready_members_.cbegin() };
        const auto e { ready_members_.cend() };

        const auto member { std::find(b, e, id) };
        if (member != e) {
            logger_.info("Membre \"{}\" [{}] n'est plus prêt.", names().at(id), id);
            ready_members_.erase(member);
        } else {
            logger_.info("Membre \"{}\" [{}] est prêt.", names().at(id), id);
            ready_members_.push_back(id);
        }

        logger_.debug("Membres prêts : {}", readyMembers());

        LobbyDataFactory ready_data;
        ready_data.makeReady(id);

        sendToAll(ready_data.dataWithLength());

        if (!isStarting() && allMembersReady() && !names().empty())
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
    sendToAll(data);

    if (names().count(master_) == 0)
        throw MasterDisconnected { master_ };
}

ReceiveBuffer Lobby::receiveFromMaster() {
    ReceiveBuffer buffer;
    buffer.fill(0);

    std::atomic_bool received { false };
    bool receive_err { false };

    connections_.at(master_).async_receive(io::buffer(buffer), [this, &received, &receive_err]
                                           (const ErrCode err, const std::size_t)
    {
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

    logger_.debug("Demande du checkpoint au master {}...", master_);
    sendToMaster(ask_data.dataWithLength());

    const ReceiveBuffer chkpt_buffer { receiveFromMaster() };
    std::string chkpt_name;

    if (chkpt_buffer[0] != 0) {
        for (std::size_t pos { 0 }; chkpt_buffer[pos] != 0; pos++)
            chkpt_name += chkpt_buffer[pos];
    }

    logger_.debug("Checkpoint : \"{}\"", chkpt_name);
    return chkpt_name;
}

bool Lobby::askYesNo(const YesNoQuestion request) {
    LobbyDataFactory ask_data;
    ask_data.makeYesNo(request);

    logger_.debug("Demande #{} au master {}...", static_cast<int>(request), master_);
    sendToMaster(ask_data.dataWithLength());

    const ReceiveBuffer reply { receiveFromMaster() };
    logger_.debug("Réponse : {}", reply[0] == 0 ? "Oui" : "Non");

    return reply[0] == 0;
}

void Lobby::launchPreparation() {
    state_ = Preparing;
    new_players_acceptor_.cancel();

    std::unique_lock cancelling_lock { request_cancelling_ };
    for (auto& connection : connections_)
        connection.second.cancel();
    cancelling_lock.unlock();

    try {
        master_ = names().cbegin()->first;

        LobbyDataFactory prepare_data;
        prepare_data.makePrepare(master_);

        logger_.info("Préparation de la session, membre master est {}.", master_);
        sendToAllMasterHandling(prepare_data.dataWithLength());

        const std::lock_guard close_guard { close_ };
        makeSession();
    } catch (const MasterDisconnected& err) {
        logger_.error(err.what());

        LobbyDataFactory master_dc_data;
        master_dc_data.makeState(State::MasterDisconnected);

        sendToAll(master_dc_data.dataWithLength());

        ready_members_.clear();
    } catch (const NoPlayerRemaining& err) {
        logger_.error(err.what());
    }

    if (!isClosed())
        open();
}

void Lobby::makeSession(std::optional<std::string> chkpt_name,
                        std::optional<bool> missing_participants)
{
    assert(!(missing_participants && chkpt_name->empty()));
    if (!chkpt_name)
        chkpt_name = askCheckpoint();
    if (!missing_participants)
        missing_participants = !chkpt_name->empty() && askYesNo(YesNoQuestion::MissingParticipants);

    Run run { runSession(*chkpt_name, *missing_participants) };
    session_.reset();
    players_.clear();
    connections_.clear();
    ready_members_.clear();

    if (run.result != SessionResult::Crashed) {
        logger_.trace("Envoi du résultat de la session aux participants.");

        for (auto& [id, participant] : run.participants) {
            players_.insert({ id, participant.name });
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

            if (names().count(master_) == 0)
                throw MasterDisconnected { master_ };

            makeSession(new_ckpt);
        }
    }
}

Run Lobby::runSession(const std::string& chkpt_name, const bool missing_participants) {
    LobbyDataFactory start_data;
    start_data.makeState(State::Start);

    logger_.info("Lancement de la session.");
    logger_.debug("checkpoint=\"{}\" ; missing_participants={}", chkpt_name, missing_participants);
    sendToAllMasterHandling(start_data.dataWithLength());

    Participants participants;
    for (const auto& [id, name] : names())
        participants.insert({ id, Particpant { name, std::move(connections_.at(id)) } });

    try {
        if (isClosed())
            return { SessionResult::Ok, std::move(participants) };

        session_.start(participants, chkpt_name, missing_participants);
        logger_.info("Fin de la session.");
    } catch (const CheckpointLoadingError& err) {
        logger_.error("Impossible de charger le checkpoint \"{}\" : {}", chkpt_name, err.what());
        return { SessionResult::CheckpointLoadingError, std::move(participants) };
    } catch (const InvalidIDs& err) {
        Run run;
        run.expectedIDs = err.expectedIDs;
        logger_.debug("expectedIDs={}", err.expectedIDs);

        if (err.errType == ParticipantsValidity::UnknownPlayer) {
            logger_.error("IDs inconnus dans les participants.");
            return { SessionResult::UnknownPlayer, std::move(participants), err.expectedIDs };
        } else {
            logger_.error("Moins de participants que prévu.");
            return { SessionResult::LessMembers, std::move(participants), err.expectedIDs };
        }
    } catch (const std::exception& err) {
        logger_.error("Session crashée : {}", err.what());
        return { SessionResult::Crashed, std::move(participants) };
    }

    return { SessionResult::Ok, std::move(participants) };
}

} // namespace Rbo::Server
