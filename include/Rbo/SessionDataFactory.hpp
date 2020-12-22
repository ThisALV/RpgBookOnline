#ifndef DATAFACTORY_HPP
#define DATAFACTORY_HPP

#include <Rbo/Data.hpp>

namespace Rbo {

enum struct DataType : byte {
    Request, Text, PlayerUpdate, GlobalStat, Die, Switch, Reply, Validation, Battle, Crash,
    LeaderSwitch, Start, Stop, FinishRequest
};

enum struct Request : byte {
    Range, Possibilities, Confirm, YesNo, DiceRoll
};

enum struct Text : byte {
    Normal, Important, Title, Note
};

enum struct ReplyValidity : byte {
    Ok, TooLate, OutOfRangeError, InavlidLengthError, NotConfirmError
};

bool isInvalid(const ReplyValidity validity);

enum struct Battle : byte {
    Init, Atk, End
};

class Data;

struct SessionDataFactory : DataFactory {
    void makeData(const DataType type);
    void makeStart(const std::string& game_name);
    void makeRequest(const Request type, const byte target);
    void makeRange(const byte target, const std::string& msg, const byte min, const byte max);
    void makePossibilities(const byte target, const std::string& msg, const OptionsList& options);
    void makeYesNoQuestion(const byte target, const std::string& question);
    void makeDiceRoll(const byte target, const std::string& msg, const byte dices, const int bonus, const DiceRollResults& results);
    void makeText(const Text txt_type);
    void makeNormalText(const std::string& txt);
    void makeImportantText(const std::string& txt);
    void makeTitle(const std::string& title);
    void makeNote(const std::string& note);
    void makePlayerUpdate(const byte id, const PlayerUpdate& changes);
    void makeGlobalStat(const std::string& name, const Stat& stat);
    void makeDie(const byte id);
    void makeSwitch(const word scene);
    void makeReply(const byte id, const byte reply);
    void makeValidation(const ReplyValidity validity);
    void makeBattle(const Battle infos);
    void makeBattleInit(const GroupDescriptor& group, const Game& ctx);
    void makeBattleAtk(const byte player, const std::string& enemy, const int dmg);
    void makeCrash(const byte id);
    void makeLeaderSwitch(const byte leader);
};

} // namespace Rbo

#endif // DATAFACTORY_HPP
