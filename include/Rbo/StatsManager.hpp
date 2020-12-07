#ifndef STATSMANAGER_HPP
#define STATSMANAGER_HPP

#include <Rbo/Common.hpp>

namespace Rbo {

struct UnknownStat : std::logic_error {
    UnknownStat(const std::string& name)
        : std::logic_error { "Unknown stat \"" + name + '"' } {}
};

struct InvalidLimit : std::logic_error {
    InvalidLimit()
        : std::logic_error { "Limits interval must contain 0 and min <= max" } {}
};

class StatsManager {
private:
    Stats stats_;

    void checkExists(const std::string& name) const;
    void readjustStat(const std::string& name);

public:
    StatsManager() = default;
    StatsManager(const std::vector<std::string>& stats_name);

    bool operator==(const StatsManager& rhs) const;

    int get(const std::string& name) const;
    void change(const std::string& name, const int relative_modification);
    void set(const std::string& name, const int new_value);

    void setLimits(const std::string& name, const int min, const int max);
    const StatLimits& limits(const std::string& name) const;

    void setHidden(const std::string& name, const bool hidden);
    bool hidden(const std::string& name) const;

    void setMain(const std::string& name, const bool main);
    bool main(const std::string& name) const;

    bool has(const std::string& stat) const { return stats_.count(stat) == 1; }

    StatsValue values() const;
    const Stats& raw() const { return stats_; }

    Stats::iterator begin() { return stats_.begin(); }
    Stats::iterator end() { return stats_.end(); }

    Stats::const_iterator cbegin() const { return stats_.cbegin(); }
    Stats::const_iterator cend() const { return stats_.cend(); }
};

} // namespace Rbo

namespace std {

template<typename Output> Output& operator<<(Output& out, const Rbo::StatsValue& stats) {
    out << '[';
    for (const auto& [name, value] : stats)
        out << " \"" << name << "\"=" << value << ';';

    out << " ]";
    return out;
}

}

#endif // STATSMANAGER_HPP
