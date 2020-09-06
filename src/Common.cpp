#include <Rbo/Common.hpp>

#include <iostream>
#include <algorithm>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <Rbo/SessionDataFactory.hpp>

#ifndef NDEBUG
#define LOG_LEVEL spdlog::level::trace
#else
#define LOG_LEVEL spdlog::level::info
#endif

namespace Rbo {

ulong now() {
    return std::chrono::system_clock::now().time_since_epoch().count();
}

namespace Sinks {

auto sink_file {
    std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                "logs/" + std::to_string(now()) + ".log", true)
};

auto sink_console { std::make_shared<spdlog::sinks::stdout_color_sink_mt>() };

const std::string log_format { "[%T.%e] %^%-8l%$ - %-11n (#%t) : %v" };

void loggerErrorHandler(const std::string& err) {
    std::cerr << "Erreur de logging : " << err << std::endl;
}

} // namespace Sinks

spdlog::logger& rboLogger(const std::string& name) {
    assert(name.length() <= 11);
    auto logger {
        std::make_shared<spdlog::logger>(name, spdlog::sinks_init_list { Sinks::sink_file, Sinks::sink_console })
    };

    spdlog::register_logger(logger);

    logger->set_level(LOG_LEVEL);
    logger->flush_on(LOG_LEVEL);
    logger->set_pattern(Sinks::log_format);
    logger->set_error_handler(Sinks::loggerErrorHandler);

    return *spdlog::get(name);
}

namespace Vote {

std::mt19937_64 rd { now() };

} // namespace Vote

byte vote(const Replies& replies) {
    assert(!replies.empty());

    std::map<byte, byte> votes_map;
    for (const auto& reply : replies)
        votes_map[reply.second]++;

    std::vector<std::pair<byte, byte>> votes {
        std::move_iterator { votes_map.begin() },
        std::move_iterator { votes_map.end() }
    };

    std::sort(votes.begin(), votes.end(), [](const auto& r1, const auto& r2) -> bool {
        return r1.second > r2.second;
    });

    const byte max_score { votes.cbegin()->second };

    std::vector<byte> winners;
    for (const auto& [option, count] : votes) {
        if (count == max_score) {
            winners.push_back(option);
            break;
        }
    }

    if (winners.size() == 1)
        return winners.at(0);

    const std::size_t winner {
        std::uniform_int_distribution<std::size_t> { 0, winners.size() - 1 } (Vote::rd)
    };

    return winners.at(winner);
}


InvalidReply::InvalidReply(const ReplyValidity err_type)
    : std::logic_error { "Invalid reply" }, type { err_type }
{
    assert(isInvalid(type));
}

std::mt19937_64 DiceFormula::rd_ { now() };
std::uniform_int_distribution<uint> DiceFormula::rd_distributor_ { 1, 6 };

int DiceFormula::operator()() const {
    int result { 0 };

    for (uint i { 0 }; i < dices; i++)
        result += rd_distributor_(rd_);

    return result + bonus;
}

namespace ItemEntry {

const char ITEM_SEP { '/' };

} // namespace ItemEntry

std::string itemEntry(const std::string& inv, const std::string& item) {
    return inv + ItemEntry::ITEM_SEP + item;
}

std::pair<std::string, std::string> splitItemEntry(const std::string& str) {
    assert(!str.empty());

    const auto cb { str.cbegin() };
    const auto ce { str.cend() };

    const auto inv_end { std::find(cb, ce, ItemEntry::ITEM_SEP) };

    assert(inv_end != ce);

    return { { cb, inv_end }, { inv_end + 1, ce } };
}

bool contains(const std::vector<std::string>& strs, const std::string& str) {
    return std::find(strs.cbegin(), strs.cend(), str) != strs.cend();
}

bool ItemBonus::operator==(const ItemBonus& rhs) const {
    return stat == rhs.stat && bonus == rhs.bonus;
}

bool StatLimits::operator==(const StatLimits& rhs) const {
    return min == rhs.min && max == rhs.max;
}

bool Stat::operator==(const Stat& rhs) const {
    return value == rhs.value && limits == rhs.limits && hidden == rhs.hidden;
}

} // namespace Rbo