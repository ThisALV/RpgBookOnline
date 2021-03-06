#include <Rbo/JsonSerialization.hpp>

namespace Rbo {

void from_json(const json& data, Messages& messages) {
    for (const auto& [name, msg] : data.get<json::object_t>())
        messages.insert({ name, msg.is_null() ? Message {} : Message { msg.get<std::string>() } });
}

void from_json(const json& data, DicesRoll& formula) {
    data.at("dices").get_to(formula.dices);
    data.at("bonus").get_to(formula.bonus);
}

void from_json(const json& data, StatDescriptor& stat) {
    const json& limits { data.at("limits") };

    data.at("initialValue").get_to(stat.initialValue);
    limits.at("min").get_to(stat.limits.min);
    limits.at("max").get_to(stat.limits.max);
    data.at("capped").get_to(stat.capped);
    data.at("hidden").get_to(stat.hidden);
    data.at("main").get_to(stat.main);
}

void from_json(const json& data, InventoryDescriptor& inv) {
    data.at("limit").get_to(inv.limit);
    data.at("items").get_to(inv.items);
    data.at("initial").get_to(inv.initialStuff);
}

void from_json(const json& data, ItemBonus& bonus) {
    data.at("stat").get_to(bonus.stat);
    data.at("bonus").get_to(bonus.bonus);
}

void from_json(const json& data, EventEffect& effect) {
    data.at("stats").get_to(effect.statsChanges);
    data.at("items").get_to(effect.itemsChanges);
}

void from_json(const json& data, EnemyDescriptor& enemy) {
    data.at("hp").get_to(enemy.hp);
    data.at("skill").get_to(enemy.skill);
}

void from_json(const json& data, EnemyDescriptorBinding& enemy) {
    data.at("ctx").get_to(enemy.ctxName);
    data.at("generic").get_to(enemy.genericName);
}

void from_json(const json& data, RestProperties& rest) {
    data.at("givables").get_to(rest.givables);
    data.at("availables").get_to(rest.availables);
}

void from_json(const json& data, Condition& condition) {
    data.at("stat").get_to(condition.stat);
    data.at("op").get_to(condition.op);
    data.at("value").get_to(condition.value);
}

void from_json(const json& data, DeathCondition& death_condition) {
    data.at("dieIf").get_to(death_condition.dieIf);
    data.at("deathMessage").get_to(death_condition.deathMessage);
}

void from_json(const json& data, EndCondition& end_condition) {
    data.at("stopIf").get_to(end_condition.stopIf);
    data.at("endMessage").get_to(end_condition.endMessage);
}

void to_json(json& data, const Stat& stat) {
    const auto [value, limits, hidden, main] { stat };

    data["value"] = value;
    data["limits"] = json::object({ { "min", limits.min }, { "max", limits.max } });
    data["hidden"] = hidden;
    data["main"] = main;
}

void from_json(const json& data, Stat& stat) {
    const json& limits { data.at("limits") };

    data.at("value").get_to(stat.value);
    limits.at("min").get_to(stat.limits.min);
    limits.at("max").get_to(stat.limits.max);
    data.at("hidden").get_to(stat.hidden);
    data.at("main").get_to(stat.main);
}

void to_json(json& data, const PlayerState& player) {
    const ToJsonWrapper<Death> death { player.death };

    data["death"] = death;
    data["stats"] = player.stats;
    data["inventories"] = player.inventories;

    // optional<T-primitif> non pris en charge par NlohmannJson
    data["capacities"] = json::object();
    for (const auto& [inventory, capacity] : player.capacities) {
        json opt;
        to_json(opt, capacity);
        data.at("capacities")[inventory] = opt;
    }
}

void from_json(const json& data, PlayerState& player) {
    FromJsonWrapper<Death> death { player.death };

    data.at("death").get_to(death);
    data.at("stats").get_to(player.stats);
    data.at("inventories").get_to(player.inventories);

    // optional<T-primitif> non pris en charge par NlohmannJson
    for (const auto& [inventory, capacity] : data.at("capacities").get<json::object_t>()) {
        InventorySize opt;
        from_json(capacity, opt);
        player.capacities[inventory] = opt;
    }
}

void to_json(json& data, const PlayerUpdate& changes) {
    const ToJsonWrapper<Death> death { changes.death };

    data["death"] = death;
    data["stats"] = json::object();
    for (const auto& [name, stat] : changes.stats) {
        if (stat.hidden)
            data.at("stats")[name] = json::object({ { "hidden", true }, { "main", stat.main } });
        else
            data.at("stats")[name] = stat;
    }

    data["inventories"] = changes.items;

    // optional<T-primitif> non pris en charge par NlohmannJson
    data["capacities"] = json::object();
    for (const auto& [inventory, capacity] : changes.capacities) {
        json opt;
        to_json(opt, capacity);
        data.at("capacities")[inventory] = opt;
    }
}

void to_json(json& data, const std::unordered_map<byte, PlayerState>& players) {
    data = json::object();
    for (const auto& [id, state] : players)
        data[std::to_string(id)] = state;
}

void from_json(const json& data, std::unordered_map<byte, PlayerState>& players) {
    for (const auto& [id, player] : data.get<json::object_t>()) {
        PlayerState state;
        player.get_to(state);

        players.insert({ static_cast<byte>(std::stoi(id)), std::move(state) });
    }
}


} // namespace Rbo
