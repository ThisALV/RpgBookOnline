#include <Rbo/Server/ServerCommon.hpp>

#include <Rbo/Data.hpp>
#include "ServerCommon.hpp"

namespace Rbo::Server {

enum struct RegistrationResult : byte {
    Ok, InvalidRequest, UnavailableID, UnavailableName, UnavailableSession
};

enum struct Event : byte {
    MemberRegistered, MemberReady, MemberDisconnected, MemberCrashed, Preparing, Prepare,
    AskCheckpoint, AskYesNo, Start, RunResult, MasterDisconnected, Open, CancelPreparing,
    SelectingCheckpoint, CheckingPlayers, RevisingParameters, MasterSwitch
};

enum struct YesNoQuestion : byte {
    MissingEntrants, RetryCheckpoint, KickUnknownPlayers
};

enum struct SessionResult : byte {
    Ok, Crashed, CheckpointLoadingError, LessMembers, UnknownPlayer, NoPlayerAlive
};

enum struct MasterSwitch : byte {
    NewMaster, NotAnyMaster
};

constexpr bool isInvalidIDs(const SessionResult result) {
    return result == SessionResult::LessMembers || result == SessionResult::UnknownPlayer;
}

constexpr bool isParametersError(const SessionResult result) {
    return result == SessionResult::CheckpointLoadingError || result == SessionResult::NoPlayerAlive || isInvalidIDs(result);
}

struct LobbyDataFactory : DataFactory {
    void makeRegistration(const RegistrationResult result);
    void makeEvent(const Event event);
    void makePreparing(const ulong delay);
    void makeNewMember(const byte id, const std::string& name);
    void makeReady(const byte id);
    void makeDisconnect(const byte id);
    void makePrepare(const byte master);
    void makeCrash(const byte id);
    void makeYesNo(const YesNoQuestion question);
    void makeResult(const SessionResult result);
    void makeRegistered(const MembersStates& members);
    void makeInvalidIDs(const SessionResult result, const std::vector<byte>& expected);
    void makeMasterSwitch(const Master& new_master);
};

} // namespace Rbo::Server
