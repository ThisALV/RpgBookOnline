#ifndef STATSMANAGER_HPP
#define STATSMANAGER_HPP

#include "Common.hpp"

namespace Rbo {

struct UnknownStat : std::logic_error {
    UnknownStat(const std::string& name)
        : std::logic_error { "Statistique \"" + name + "\" inconnue" } {}
};

struct InvalidLimit : std::logic_error {
    InvalidLimit()
        : std::logic_error { "L'intervale limite doit contenir 0 et min <= max" } {}
};

class StatsManager {
private:
    Stats stats_;

    void checkExists(const std::string&) const;
    void readjustStat(const std::string&);

public:
    StatsManager() = default;
    StatsManager(const std::vector<std::string>&);

    bool operator==(const StatsManager&) const;

    int get(const std::string&) const;
    void change(const std::string&, const int);
    void set(const std::string&, const int);

    void setLimits(const std::string&, const int, const int);
    const StatLimits& limits(const std::string&) const;

    void setHidden(const std::string&, const bool);
    bool hidden(const std::string&) const;

    bool has(const std::string& stat) const { return stats_.count(stat) == 1; }

    StatsValues values() const;
    const Stats& raw() const { return stats_; }

    Stats::iterator begin() { return stats_.begin(); }
    Stats::iterator end() { return stats_.end(); }

    Stats::const_iterator cbegin() const { return stats_.cbegin(); }
    Stats::const_iterator cend() const { return stats_.cend(); }
};

} // namespace Rbo

#endif // STATSMANAGER_HPP
