#ifndef JSONSERIALIZATION_HPP
#define JSONSERIALIZATION_HPP

#include <Rbo/Common.hpp>

#include <algorithm>
#include <nlohmann/json.hpp>
#include <Rbo/Game.hpp>

namespace Rbo { // Doivent être dans le même ns que leur type pour fonctionner avec Json

using json = nlohmann::json;

template<typename T>
void to_json(json& data, const std::optional<T>& opt) {
    data = opt ? json(*opt) : json(nullptr);
}

template<typename T>
void from_json(const json& data, std::optional<T>& opt) {
    opt = data.is_null() ? std::optional<T> {} : data.get<T>();
}

void from_json(const json& data, Messages& messages);

// Pour pouvoir utiliser from_json avec des using dans Rbo sur des types de la std en utilisant l'ADL
template<typename Serializable>
struct FromJsonWrapper {
    Serializable& value;
};

template<typename Serializable>
void from_json(const json& data, FromJsonWrapper<Serializable>& serializable) {
    from_json(data, serializable.value);
}

void from_json(const json& data, DicesRoll& formula);
void from_json(const json& data, StatDescriptor& stat);
void from_json(const json& data, InventoryDescriptor& inv);
void from_json(const json& data, ItemBonus& bonus);
void from_json(const json& data, EventEffect& effect);
void from_json(const json& data, EnemyDescriptor& enemy);
void from_json(const json& data, GroupDescriptor& group);
void from_json(const json& data, RestProperties& rest);
void from_json(const json& data, Condition& condition);
void from_json(const json& data, DeathCondition& death_condition);
void from_json(const json& data, EndCondition& end_condition);

void to_json(json& data, const Stat& stat);
void from_json(const json& data, Stat& stat);

void to_json(json& data, const PlayerState& player);
void from_json(const json& data, PlayerState& player);

void to_json(json& data, const std::map<byte, PlayerState>& players);
void from_json(const json& data, std::map<byte, PlayerState>& players);

void to_json(json& data, const PlayerUpdate& changes);

} // namespace Rbo

#endif // JSONSERIALIZATION_HPP
