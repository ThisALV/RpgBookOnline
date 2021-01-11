#include <Rbo/Server/LobbyDataFactory.hpp>

namespace Rbo::Server {

void LobbyDataFactory::makeRegistration(const RegistrationResult result) {
    data_.add(result);
}

void LobbyDataFactory::makeEvent(const Event event) {
    data_.add(event);
}

void LobbyDataFactory::makePreparing(const ulong delay) {
    makeEvent(Event::BeginCountdown);
    data_.putNumeric(delay);
}

void LobbyDataFactory::makeNewMember(const byte id, const std::string& name) {
    makeEvent(Event::MemberRegistered);
    data_.add(id);
    data_.put(name);
}

void LobbyDataFactory::makeReady(const byte id) {
    makeEvent(Event::MemberReady);
    data_.add(id);
}

void LobbyDataFactory::makeDisconnect(const byte id) {
    makeEvent(Event::MemberDisconnected);
    data_.add(id);
}

void LobbyDataFactory::makePrepare(const byte id) {
    makeEvent(Event::SessionPreparation);
    data_.add(id);
}

void LobbyDataFactory::makeCrash(const byte id) {
    makeEvent(Event::MemberCrashed);
    data_.add(id);
}

void LobbyDataFactory::makeResult(const SessionResult result) {
    makeEvent(Event::RunResult);
    data_.add(result);
}

void LobbyDataFactory::makeYesNo(const YesNoQuestion request) {
    makeEvent(Event::AskYesNo);
    data_.add(request);
}

void LobbyDataFactory::makeRegistered(const MembersStates& members) {
    makeRegistration(RegistrationResult::Ok);

    data_.add(members.size());
    for (const auto& [id, member] : members) {
        data_.add(id);
        data_.put(member.name);
        data_.add(member.ready);
    }
}

void LobbyDataFactory::makeInvalidIDs(const SessionResult result, const std::vector<byte>& expected) {
    assert(isInvalidIDs(result));

    makeResult(result);

    data_.add(expected.size());
    for (const byte id : expected)
        data_.add(id);
}

void LobbyDataFactory::makeMasterSwitch(const Master& new_master) {
    makeEvent(Event::MasterSwitch);

    if (new_master.exists()) {
        data_.add(MasterSwitch::NewMaster);
        data_.add(new_master.load());
    } else {
        data_.add(MasterSwitch::NotAnyMaster);
    }
}

} // namespace Rbo::Server
