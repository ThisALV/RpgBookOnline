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

inline void to_json(json& data, const DiceFormula& formula) {
    data["dices"] = formula.dices;
    data["bonus"] = formula.bonus;
}

inline void from_json(const json& data, DiceFormula& formula) {
    data.at("dices").get_to(formula.dices);
    data.at("bonus").get_to(formula.bonus);
}

inline void to_json(json& data, const StatInitilizer& stat) {
    const auto [formula, limits, capped, hidden] { stat };

    data["formula"] = formula;
    data["limits"] = json::object({ { "min", limits.min }, { "max", limits.max } });
    data["capped"] = capped;
    data["hidden"] = hidden;
}

inline void from_json(const json& data, StatInitilizer& stat) {
    const json& limits { data.at("limits") };

    data.at("formula").get_to(stat.initialValue);
    limits.at("min").get_to(stat.limits.min);
    limits.at("max").get_to(stat.limits.max);
    data.at("capped").get_to(stat.capped);
    data.at("hidden").get_to(stat.hidden);
}

inline void to_json(json& data, const InventoryInitializer& inv) {
    data["limit"] = inv.limit;
    data["items"] = inv.items;
    data["initial"] = inv.initialStuff;
}

inline void from_json(const json& data, InventoryInitializer& inv) {
    data.at("limit").get_to(inv.limit);
    data.at("items").get_to(inv.items);
    data.at("initial").get_to(inv.initialStuff);
}

inline void to_json(json& data, const ItemBonus& bonus) {
    data["stat"] = bonus.stat;
    data["bonus"] = bonus.bonus;
}

inline void from_json(const json& data, ItemBonus& bonus) {
    data.at("stat").get_to(bonus.stat);
    data.at("bonus").get_to(bonus.bonus);
}

inline void to_json(json& data, const EventEffect& effect) {
    data["stats"] = effect.statsChanges;
    data["items"] = effect.itemsChanges;
}

inline void from_json(const json& data, EventEffect& effect) {
    data.at("stats").get_to(effect.statsChanges);
    data.at("items").get_to(effect.itemsChanges);
}

inline void to_json(json& data, const RestProperties& rest) {
    data["givables"] = rest.givables;
    data["availables"] = rest.availables;
}

inline void from_json(const json& data, RestProperties& rest) {
    data.at("givables").get_to(rest.givables);
    data.at("availables").get_to(rest.availables);
}

inline void to_json(json& data, const Condition& condition) {
    data["stat"] = condition.stat;
    data["op"] = condition.op;
    data["value"] = condition.value;
}

inline void from_json(const json& data, Condition& condition) {
    data.at("stat").get_to(condition.stat);
    data.at("op").get_to(condition.op);
    data.at("value").get_to(condition.value);
}

} // namespace Rbo::Server

#endif // GAMEJSONCAST_INL
