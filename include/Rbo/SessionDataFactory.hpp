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

enum struct ReplyValidity : byte {
    Ok, TooLate, OutOfRangeError, InavlidLengthError, NotConfirmError
};

bool isInvalid(const ReplyValidity validity);

enum struct BattleInfo : byte {
    Init, Atk, End
};

class Data;

struct SessionDataFactory : DataFactory {
    void makeData(const DataType type);
    void makeStart(const std::string& game_name);
    void makeRequest(const Request type);
    void makeRange(const std::string& msg, const byte min, const byte max);
    void makePossibilities(const std::string& msg, const OptionsList& options);
    void makeYesNoQuestion(const std::string& question);
    void makeText(const std::string& txt);
    void makeInfos(const byte id, const PlayerStateChanges& changes);
    void makeGlobalStat(const std::string& name, const Stat& stat);
    void makeDie(const byte id);
    void makeSwitch(const word scene);
    void makeReply(const byte id, const byte reply);
    void makeValidation(const ReplyValidity validity);
    void makeBattleInfos(const BattleInfo infos);
    void makeBattleInit(const GroupDescriptor& group, const std::unordered_map<std::string, EnemyDescriptor>& enemies_db);
    void makeAtk(const byte player, const std::string& enemy, const int dmg);
    void makeCrash(const byte id);
    void makeLeaderSwitch(const byte leader);
};

} // namespace Rbo

#endif // DATAFACTORY_HPP
