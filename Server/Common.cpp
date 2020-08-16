#include "Common.hpp"

#include <iostream>
#include <cassert>
#include <random>
#include <algorithm>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "SessionDataFactory.hpp"

#ifndef NDEBUG
#define LOG_LEVEL spdlog::level::trace
#else
#define LOG_LEVEL spdlog::level::info
#endif

namespace Rbo {

auto sink_file {
    std::make_shared<spdlog::sinks::basic_file_sink_mt>(
                "logs/"
                + std::to_string(std::chrono::system_clock::now().time_since_epoch().count())
                + ".log", true)
};

auto sink_console { std::make_shared<spdlog::sinks::stdout_color_sink_mt>() };

const std::string log_format { "[%T.%e] %^%-8l%$ - %-11n (#%t) : %v" };

void loggerErrorHandler(const std::string& err) {
    std::cerr << "Erreur de logging : " << err << std::endl;
}

spdlog::logger& rboLogger(const std::string& name) {
    assert(name.length() <= 11);
    auto logger {
        std::make_shared<spdlog::logger>(name, spdlog::sinks_init_list { sink_file, sink_console })
    };

    spdlog::register_logger(logger);

    logger->set_level(LOG_LEVEL);
    logger->flush_on(LOG_LEVEL);
    logger->set_pattern(log_format);
    logger->set_error_handler(loggerErrorHandler);

    return *spdlog::get(name);
}

std::default_random_engine rd;

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
        std::uniform_int_distribution<std::size_t>{ 0, winners.size() - 1 }(rd)
    };

    return winners.at(winner);
}


InvalidReply::InvalidReply(const ReplyValidity err_type)
    : std::logic_error { "Invalid reply" }, type { err_type }
{
    assert(isInvalid(type));
}

std::default_random_engine rd_engine;
std::uniform_int_distribution<uint> rd_distributor { 1, 6 };

int DiceFormula::operator()() const {
    int result { 0 };

    for (uint i { 0 }; i < dices; i++)
        result += rd_distributor(rd_engine);

    return result + bonus;
}

const char ITEM_SEP { '/' };

std::string itemEntry(const std::string& inv, const std::string& item) {
    return inv + ITEM_SEP + item;
}

std::pair<std::string, std::string> splitItemEntry(const std::string& str) {
    assert(!str.empty());

    const auto cb { str.cbegin() };
    const auto ce { str.cend() };

    const auto inv_end { std::find(cb, ce, ITEM_SEP) };

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
