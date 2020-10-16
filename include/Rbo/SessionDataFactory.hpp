#ifndef DATAFACTORY_HPP
#define DATAFACTORY_HPP

#include <Rbo/Data.hpp>

namespace Rbo {

enum struct DataType : byte {
    Request, Text, Infos, GlobalStat, Die, Switch, Reply, Validation, BattleInfos, Crash,
    LeaderSwitch, Start, Stop
};

enum struct Request : byte {
    Range, Possibilities, Confirm, YesNo, End
};

enum struct Text : byte {
    Plain, List
};

enum struct ReplyValidity : byte {
    Ok, TooLate, OutOfRangeError, InavlidLengthError, NotConfirmError
};

bool isInvalid(const ReplyValidity);

enum struct BattleInfo : byte {
    Init, Atk, End
};

class Data;

struct SessionDataFactory : DataFactory {
    void makeData(const DataType);
    void makeRequest(const Request);
    void makeRange(const byte, const byte);
    void makePossibilities(const std::vector<byte>&);
    void makeText(const Text);
    void makePlain(const std::string&);
    void makeOptions(const OptionsList&);
    void makeInfos(const byte, const PlayerStateChanges&);
    void makeGlobalStat(const std::string&, const Stat&);
    void makeDie(const byte);
    void makeSwitch(const word);
    void makeReply(const byte, const byte);
    void makeValidation(const ReplyValidity);
    void makeBattleInfos(const BattleInfo);
    void makeBattleInit(const GroupDescriptor&, const std::unordered_map<std::string, EnemyDescriptor>&);
    void makeAtk(const byte, const std::string&, const int);
    void makeCrash(const byte);
    void makeLeaderSwitch(const byte);
};

} // namespace Rbo

#endif // DATAFACTORY_HPP
