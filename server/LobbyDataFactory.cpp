#include <Rbo/Server/LobbyDataFactory.hpp>

#include <cassert>

namespace Rbo::Server {

bool isParametersError(const SessionResult result) {
    return static_cast<byte>(result) > 1;
}

bool isInvalidIDs(const SessionResult result) {
    return static_cast<byte>(result) > 2;
}

void LobbyDataFactory::makeRegistration(const RegistrationResult result) {
    data_.add(result);
}

void LobbyDataFactory::makeState(const State state) {
    data_.add(state);
}

void LobbyDataFactory::makeNewMember(const byte id, const std::string& name) {
    makeState(State::MemberRegistered);
    data_.add(id);
    data_.put(name);
}

void LobbyDataFactory::makeReady(const byte id) {
    makeState(State::MemberReady);
    data_.add(id);
}

void LobbyDataFactory::makeDisconnect(const byte id) {
    makeState(State::MemberDisconnected);
    data_.add(id);
}

void LobbyDataFactory::makePrepare(const byte id) {
    makeState(State::Prepare);
    data_.add(id);
}

void LobbyDataFactory::makeCrash(const byte id) {
    makeState(State::MemberCrashed);
    data_.add(id);
}

void LobbyDataFactory::makeResult(const SessionResult result) {
    makeState(State::RunResult);
    data_.add(result);
}

void LobbyDataFactory::makeYesNo(const YesNoQuestion request) {
    makeState(State::AskYesNo);
    data_.add(request);
}

void LobbyDataFactory::makeInvalidIDs(const SessionResult result,
                                      const std::vector<byte>& expected)
{
    assert(isInvalidIDs(result));

    makeResult(result);

    data_.add(expected.size());
    for (const byte id : expected)
        data_.add(id);
}

} // namespace Rbo::Server
