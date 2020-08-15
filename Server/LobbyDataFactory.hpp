#include "Data.hpp"

namespace Rbo::Server {

enum struct RegistrationResult : byte {
    Ok, InvalidRequest, UnavailableID, UnavailableName, UnavailableSession
};

enum struct State : byte {
    MemberRegistered, MemberReady, MemberDisconnected, MemberCrashed, Preparing, Prepare,
    AskCheckpoint, AskYesNo, Start, RunResult, MasterDisconnected, Open
};

enum struct YesNoQuestion : byte {
    MissingParticipants, RetryCheckpoint, KickUnknownPlayers
};

enum struct SessionResult : byte {
    Ok, Crashed, CheckpointLoadingError, LessMembers, UnknownPlayers
};

bool isParametersError(const SessionResult);
bool isInvalidIDs(const SessionResult);

struct LobbyDataFactory : DataFactory {
    void makeRegistration(const RegistrationResult);
    void makeState(const State);
    void makeNewMember(const byte, const std::string&);
    void makeReady(const byte);
    void makeDisconnect(const byte);
    void makePrepare(const byte);
    void makeCrash(const byte);
    void makeYesNo(const YesNoQuestion);
    void makeResult(const SessionResult);
    void makeInvalidIDs(const SessionResult, const std::vector<byte>&);
};

} // namespace Rbo::Server
