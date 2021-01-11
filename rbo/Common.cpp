#include <Rbo/Common.hpp>

#include <memory>
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

namespace {

auto sink_file { std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/" + std::to_string(now()) + ".log", true) };
auto sink_console { std::make_shared<spdlog::sinks::stdout_color_sink_mt>() };

constexpr std::string_view log_format { "[%T.%e/%^%L%$] %-13n #%t : %v" };

void loggerErrorHandler(const std::string& err) {
    std::cerr << "Logger error : " << err << std::endl;
}

}

spdlog::logger& rboLogger(std::string name) {
    assert(name.length() <= 11);
    auto logger { std::make_shared<spdlog::logger>(std::move(name), spdlog::sinks_init_list { sink_file, sink_console }) };

    spdlog::register_logger(logger);

    logger->set_level(LOG_LEVEL);
    logger->flush_on(LOG_LEVEL);
    logger->set_pattern(std::string { log_format });
    logger->set_error_handler(loggerErrorHandler);

    return *logger;
}

namespace {

RandomEngine vote_rd { now() };

}

std::optional<byte> vote(const Replies& replies) {
    if (replies.empty())
        return {};

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
        if (count == max_score)
            winners.push_back(option);
    }

    if (winners.size() == 1)
        return winners.at(0);

    const std::size_t winner { std::uniform_int_distribution<std::size_t> { 0, winners.size() - 1 } (vote_rd) };

    return winners.at(winner);
}


InvalidReply::InvalidReply(const ReplyValidity err_type) : std::logic_error { "Invalid reply" }, type { err_type } {
    assert(isInvalid(type));
}

int RollResult::total() const {
    return std::accumulate(dices.cbegin(), dices.cend(), 0) + bonus;
}

namespace  {

RandomEngine dices_rd { now() };
std::uniform_int_distribution<uint> dices_rd_distributor { 1, 6 };

}

RollResult DicesRoll::operator()() const {
    RollResult result;
    result.dices.reserve(dices);
    result.bonus = bonus;

    for (uint i { 0 }; i < dices; i++) {
        const uint dice_result { dices_rd_distributor(dices_rd) };
        result.dices.push_back(dice_result);
    }

    return result;
}

std::string itemEntry(const std::string& inv, const std::string& item) {
    return inv + ITEM_ENTRY_SEP + item;
}

std::pair<std::string, std::string> splitItemEntry(const std::string& str) {
    assert(!str.empty());

    const auto cb { str.cbegin() };
    const auto ce { str.cend() };

    const auto inv_end { std::find(cb, ce, ITEM_ENTRY_SEP) };

    assert(inv_end != ce);

    return { { cb, inv_end }, { inv_end + 1, ce } };
}

bool ItemBonus::operator==(const ItemBonus& rhs) const {
    return stat == rhs.stat && bonus == rhs.bonus;
}

bool StatLimits::operator==(const StatLimits& rhs) const {
    return min == rhs.min && max == rhs.max;
}

bool Stat::operator==(const Stat& rhs) const {
    return value == rhs.value && limits == rhs.limits && hidden == rhs.hidden && main == rhs.main;
}

} // namespace Rbo
