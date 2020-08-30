#ifndef GAMEJSONCAST_INL
#define GAMEJSONCAST_INL

#include "Common.hpp"

#include "nlohmann/json.hpp"
#include "Game.hpp"

namespace Rbo { // Doivent être dans le même ns que leur type pour fonctionner avec Json

using json = nlohmann::json;

template<typename T> void to_json(json& data, const std::optional<T>& opt) {
    data = opt ? json { *opt } : json { nullptr };
}

template<typename T> void from_json(const json& data, std::optional<T>& opt) {
    opt = data.is_null() ? std::optional<T> {} : data.get<T>();
}

inline void from_json(const json& data, DiceFormula& formula) {
    data.at("dices").get_to(formula.dices);
    data.at("bonus").get_to(formula.bonus);
}

inline void from_json(const json& data, StatInitilizer& stat) {
    const json& limits { data.at("limits") };

    data.at("formula").get_to(stat.initialValue);
    limits.at("min").get_to(stat.limits.min);
    limits.at("max").get_to(stat.limits.max);
    data.at("capped").get_to(stat.capped);
    data.at("hidden").get_to(stat.hidden);
}

inline void from_json(const json& data, InventoryDescriptor& inv) {
    data.at("limit").get_to(inv.limit);
    data.at("items").get_to(inv.items);
    data.at("initial").get_to(inv.initialStuff);
}

inline void from_json(const json& data, ItemBonus& bonus) {
    data.at("stat").get_to(bonus.stat);
    data.at("bonus").get_to(bonus.bonus);
}

inline void from_json(const json& data, EventEffect& effect) {
    data.at("stats").get_to(effect.statsChanges);
    data.at("items").get_to(effect.itemsChanges);
}

inline void from_json(const json& data, RestProperties& rest) {
    data.at("givables").get_to(rest.givables);
    data.at("availables").get_to(rest.availables);
}

inline void from_json(const json& data, Condition& condition) {
    data.at("stat").get_to(condition.stat);
    data.at("op").get_to(condition.op);
    data.at("value").get_to(condition.value);
}

inline void to_json(json& data, const Stat& stat) {
    const auto [value, limits, hidden] { stat };

    data["value"] = value;
    data["limits"] = json::object({ { "min", limits.min }, { "max", limits.max } });
    data["hidden"] = hidden;
}

inline void from_json(const json& data, Stat& stat) {
    const json& limits { data.at("limits") };

    data.at("value").get_to(stat.value);
    limits.at("min").get_to(stat.limits.min);
    limits.at("max").get_to(stat.limits.max);
    data.at("hidden").get_to(stat.hidden);
}

inline void to_json(json& data, const PlayerState& player) {
    data["stats"] = player.stats;
    data["inventories"] = player.inventories;

    // optional<T-primitif> non pris en charge par NlohmannJson
    data["inventoriesMaxCapacity"] = json::object();
    for (const auto& [inventory, capacity] : player.inventoriesMaxCapacity) {
        json opt;
        to_json(opt, capacity);
        data["inventoriesMaxCapacity"][inventory] = opt;
    }
}

inline void from_json(const json& data, PlayerState& player) {
    data.at("stats").get_to(player.stats);
    data.at("inventories").get_to(player.inventories);

    // optional<T-primitif> non pris en charge par NlohmannJson
    for (const auto& [inventory, capacity] : data.at("inventoriesMaxCapacity").get<json::object_t>()) {
        InventorySize opt;
        from_json(capacity, opt);
        player.inventoriesMaxCapacity[inventory] = opt;
    }
}

} // namespace Rbo::Server

#endif // GAMEJSONCAST_INL
