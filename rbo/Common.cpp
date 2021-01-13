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

constexpr std::string_view log_format { "[%T.%e/%^%L%$] %-13n #%-5t : %v" };

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

namespace { RandomEngine vote_rd { now() }; }

std::optional<byte> vote(const Replies& replies) {
    if (replies.empty())
        return {};

    std::unordered_map<byte, byte> replies_score;
    replies_score.reserve(replies.size());

    for (const auto [player, reply] : replies)
        replies_score[reply]++;

    std::vector<byte> best_replies;
    best_replies.reserve(replies_score.size());

    byte best_score { 0 };
    for (const auto [reply, score] : replies_score) {
        if (score == best_score) {
            best_replies.push_back(reply);
        } else if (score > best_score) {
            best_replies.clear();
            best_replies.push_back(reply);
            best_score = score;
        }
    }

    if (best_replies.size() == 1)
        return best_replies.at(0);

    const std::size_t winner_i { std::uniform_int_distribution<std::size_t> { 0, best_replies.size() } (vote_rd) };
    return best_replies.at(winner_i);
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
