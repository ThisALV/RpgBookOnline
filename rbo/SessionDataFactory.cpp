#include <Rbo/SessionDataFactory.hpp>

#include <nlohmann/json.hpp>
#include <Rbo/GameJsonCast.hpp>

namespace Rbo {

using json = nlohmann::json;

bool isInvalid(const ReplyValidity type) {
    return static_cast<byte>(type) > 1;
}

void SessionDataFactory::makeData(const DataType type) {
    data_.add(type);
}

void SessionDataFactory::makeStart(const std::string& game_name) {
    makeData(DataType::Start);
    data_.put(game_name);
}

void SessionDataFactory::makeRequest(const Request type, const byte target) {
    makeData(DataType::Request);
    data_.add(type);
    data_.add(target);
}

void SessionDataFactory::makeRange(const byte target, const std::string& msg, const byte min, const byte max) {
    makeRequest(Request::Range, target);
    data_.put(msg);
    data_.add(min);
    data_.add(max);
}

void SessionDataFactory::makePossibilities(const byte target, const std::string& msg, const OptionsList& options) {
    makeRequest(Request::Possibilities, target);
    data_.put(msg);
    data_.putList(options);
}

void SessionDataFactory::makeYesNoQuestion(const byte target, const std::string& question) {
    makeRequest(Request::YesNo, target);
    data_.put(question);
}

void SessionDataFactory::makeDiceRoll(const byte target, const std::string &msg, const byte dices, const int bonus, const DiceRollResults& results) {
    makeRequest(Request::DiceRoll, target);
    data_.put(msg);
    data_.add(dices);
    data_.putNumeric(bonus);

    data_.add(results.size());
    for (const auto [id, result] : results) {
        data_.add(id);

        for (const byte dice : result.dices)
            data_.add(dice);
    }
}

void SessionDataFactory::makeText(const Text txt_type) {
    makeData(DataType::Text);
    data_.add(txt_type);
}

void SessionDataFactory::makeNormalText(const std::string& txt) {
    makeText(Text::Normal);
    data_.put(txt);
}

void SessionDataFactory::makeImportantText(const std::string& txt) {
    makeText(Text::Important);
    data_.put(txt);
}

void SessionDataFactory::makeTitle(const std::string& title) {
    makeText(Text::Title);
    data_.put(title);
}

void SessionDataFactory::makeNote(const std::string& note) {
    makeText(Text::Note);
    data_.put(note);
}

void SessionDataFactory::makePlayerUpdate(const byte id, const PlayerUpdate& changes) {
    // Ce type de constructeur est utilisé car {} ferait appelle à une initializer_list et = à une convertion implicite, ce qui n'est pas souhaitable.
    const json changes_data(changes);

    makeData(DataType::PlayerUpdate);
    data_.add(id);

    data_.put(changes_data.dump());
}

void SessionDataFactory::makeGlobalStat(const std::string& name, const Stat& stat) {
    makeData(DataType::GlobalStat);
    data_.put(name);
    data_.add(stat.hidden);
    data_.add(stat.main);

    if (stat.hidden)
        return;

    data_.putNumeric(stat.limits.min);
    data_.putNumeric(stat.limits.max);
    data_.putNumeric(stat.value);
}

void SessionDataFactory::makeDie(const byte id, const std::string& reason) {
    makeData(DataType::Die);
    data_.add(id);
    data_.put(reason);
}

void SessionDataFactory::makeSwitch(const word id) {
    makeData(DataType::Switch);
    data_.putNumeric(id);
}

void SessionDataFactory::makeReply(const byte id, const byte reply) {
    makeData(DataType::Reply);
    data_.add(id);
    data_.add(reply);
}

void SessionDataFactory::makeValidation(const ReplyValidity reply) {
    makeData(DataType::Validation);
    data_.add(reply);
}

void SessionDataFactory::makeBattle(const Battle type) {
    makeData(DataType::Battle);
    data_.add(type);
}

void SessionDataFactory::makeBattleInit(const GroupDescriptor& group, const Game& ctx) {
    json infos;
    for (const auto& [priority, enemy_descriptor_binding] : group) {
        const auto& [ctxName, genericName] { enemy_descriptor_binding };
        const EnemyDescriptor& enemy { ctx.enemy(genericName) };

        infos[std::to_string(priority)] = {
            { "name", ctxName },
            { "hp", enemy.hp },
            { "skill", enemy.skill }
        };
    }

    makeBattle(Battle::Init);
    data_.put(infos.dump());
}

void SessionDataFactory::makeBattleAtk(const byte player, const std::string& enemy, const int dmg) {
    makeBattle(Battle::Atk);
    data_.add(player);
    data_.put(enemy);
    data_.putNumeric(dmg);
}

void SessionDataFactory::makeCrash(const byte player) {
    makeData(DataType::Crash);
    data_.add(player);
}

void SessionDataFactory::makeLeaderSwitch(const byte player) {
    makeData(DataType::LeaderSwitch);
    data_.add(player);
}

} // namespace Rbo
