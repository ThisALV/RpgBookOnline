#include <Rbo/StatsManager.hpp>

#include <cassert>

namespace Rbo {

StatsManager::StatsManager(const std::vector<std::string>& stats) {
    std::transform(stats.cbegin(), stats.cend(), std::inserter(stats_, stats_.begin()), [](const std::string& stat) {
        return Stats::value_type { stat, {} };
    });
}

bool StatsManager::operator==(const StatsManager& rhs) const {
    return raw() == rhs.raw();
}

void StatsManager::checkExists(const std::string& name) const {
    if (!has(name))
        throw UnknownStat { name };
}

void StatsManager::readjustStat(const std::string& name) {
    Stat& stat { stats_.at(name) };
    const auto [ min, max ] { stat.limits };
    int& value { stat.value };

    if (value > max)
        value = max;
    else if (value < min)
        value = min;
}

void StatsManager::set(const std::string& stat, const int value) {
    checkExists(stat);
    stats_.at(stat).value = value;
    readjustStat(stat);
}

void StatsManager::change(const std::string& stat, const int modification) {
    checkExists(stat);
    stats_.at(stat).value += modification;
    readjustStat(stat);
}

int StatsManager::get(const std::string& stat) const {
    checkExists(stat);
    return raw().at(stat).value;
}

void StatsManager::setLimits(const std::string& stat, const int min, const int max) {
    checkExists(stat);
    if (min > 0 || max < 0)
        throw InvalidLimit {};

    stats_.at(stat).limits = { min, max };
    readjustStat(stat);
}

const StatLimits& StatsManager::limits(const std::string& stat) const {
    checkExists(stat);
    return stats_.at(stat).limits;
}

void StatsManager::setHidden(const std::string& stat, const bool hidden) {
    checkExists(stat);
    stats_.at(stat).hidden = hidden;
}

bool StatsManager::hidden(const std::string& stat) const {
    checkExists(stat);
    return stats_.at(stat).hidden;
}

void StatsManager::setMain(const std::string& stat, const bool main) {
    checkExists(stat);
    stats_.at(stat).main = main;
}

bool StatsManager::main(const std::string& stat) const {
    checkExists(stat);
    return stats_.at(stat).main;
}

StatsValues StatsManager::values() const {
    StatsValues values;
    std::transform(stats_.cbegin(), stats_.cend(), std::inserter(values, values.begin()), [](const auto& s) { return StatsValues::value_type { s.first, s.second.value }; });

    return values;
}

}
