#include <Rbo/SessionDataFactory.hpp>

#include <nlohmann/json.hpp>

namespace Rbo {

using json = nlohmann::json;

bool isInvalid(const ReplyValidity type) {
    return static_cast<byte>(type) > 1;
}

void SessionDataFactory::makeData(const DataType type) {
    data_.add(type);
}

void SessionDataFactory::makeRequest(const Request type) {
    makeData(DataType::Request);
    data_.add(type);
}

void SessionDataFactory::makeRange(const byte min, const byte max) {
    makeRequest(Request::Range);
    data_.add(min);
    data_.add(max);
}

void SessionDataFactory::makePossibilities(const std::vector<byte>& range) {
    makeRequest(Request::Possibilities);
    data_.add(range.size());

    for (const byte possibility : range)
        data_.add(possibility);
}

void SessionDataFactory::makeText(const Text type) {
    makeData(DataType::Text);
    data_.add(type);
}

void SessionDataFactory::makePlain(const std::string& txt) {
    makeText(Text::Plain);
    data_.put(txt);
}

void SessionDataFactory::makeOptions(const OptionsList& options) {
    makeText(Text::List);
    data_.add(static_cast<byte>(options.size())); // static_cast éviter d'utiliser le template add

    for (const auto& [option, txt] : options) {
        data_.add(option);
        data_.put(txt);
    }
}

void SessionDataFactory::makeInfos(const byte id, const PlayerStateChanges& changes) {
    json changes_data = json::object({
        { "inventories", changes.itemsChanges },
        { "inventoriesMaxCapacity", changes.capacitiesChanges },
        { "stats", json::object({}) }
    });

    for (const auto& [name, stat] : changes.statsChanges) {
        assert(!stat.hidden);

        changes_data["stats"][name] = json::object({
            { "limits", { { "min", stat.limits.min }, { "max", stat.limits.max } } },
            { "value", stat.value },
            { "hidden", stat.hidden }
        });
    }

    makeData(DataType::Infos);
    data_.add(id);

    data_.put(changes_data.dump());
}

void SessionDataFactory::makeGlobalStat(const std::string stat, const int min, const int max,
                                        const int value, const bool hidden)
{
    makeData(DataType::GlobalStat);
    data_.put(stat);
    data_.putNumeric(min);
    data_.putNumeric(max);
    data_.putNumeric(value);
    data_.add(hidden ? 1 : 0);
}

void SessionDataFactory::makeDie(const byte id) {
    makeData(DataType::Die);
    data_.add(id);
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

void SessionDataFactory::makeBattleInfos(const BattleInfo type) {
    makeData(DataType::BattleInfos);
    data_.add(type);
}

void SessionDataFactory::makeBattleInit(
        const GroupDescriptor& group,
        const std::unordered_map<std::string, EnemyDescriptor>& enemies)
{
    json infos;
    for (const auto& [name, generic_name] : group) {
        const EnemyDescriptor& enemy { enemies.at(generic_name) };

        infos[name] = {
            { "hp", enemy.hp },
            { "skill", enemy.skill }
        };
    }

    makeBattleInfos(BattleInfo::Init);
    data_.put(infos.dump());
}

void SessionDataFactory::makeAtk(const byte player, const std::string& enemy,
                                 const int dmg)
{
    makeBattleInfos(BattleInfo::Atk);
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
