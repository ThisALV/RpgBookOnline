#include <Rbo/Server/Common.hpp>

#include <Rbo/Data.hpp>

namespace Rbo::Server {

enum struct RegistrationResult : byte {
    Ok, InvalidRequest, UnavailableID, UnavailableName, UnavailableSession
};

enum struct Event : byte {
    MemberRegistered    = 0,
    MemberReady         = 1,
    MemberDisconnected  = 2,
    MemberCrashed       = 3,
    BeginCountdown      = 4,
    SessionPreparation  = 5,
    AskCheckpoint       = 6,
    AskYesNo            = 7,
    Start               = 8,
    RunResult           = 9,
    MasterDisconnected  = 10,
    Open                = 11,
    CancelCountdown     = 12,
    SelectingCheckpoint = 13,
    CheckingPlayers     = 14,
    RevisingParameters  = 15,
    MasterSwitch        = 16
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
