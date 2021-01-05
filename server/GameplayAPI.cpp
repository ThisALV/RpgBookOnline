#include <Rbo/Server/InstructionsProvider.hpp>

#include <Rbo/Gameplay.hpp>
#include <Rbo/Game.hpp>
#include <Rbo/Player.hpp>

namespace Rbo::Server {

namespace {

std::vector<byte> getIDs(const Replies& replies) {
    std::vector<byte> ids;
    ids.resize(replies.size(), 0);

    std::transform(replies.cbegin(), replies.cend(), ids.begin(), [](const auto r) {
        return r.first;
    });

    return ids;
}

std::tuple<byte, byte> reply(const Replies& replies) {
    if (replies.size() != 1)
        throw std::invalid_argument { "To retrieve an unique reply, there must be one" };

    const auto reply { replies.cbegin() };
    return std::make_tuple(reply->first, reply->second);
}

void assertArgs(const bool assertion) {
    if (!assertion)
        throw std::invalid_argument { "Invalid arguments" };
}

bool toBoolean(const std::string& str) {
    if (str == "true")
        return true;
    else if (str == "false")
        return false;

    throw std::invalid_argument { "Unable to convert \"" + str + "\" into boolean" };
}

void applyToGlobal(Gameplay& ctx, const EventEffect& effect) {
    if (!effect.itemsChanges.empty())
        throw std::logic_error { "Session doesn't have any items neither inventories" };

    for (const auto& [stat, change] : effect.statsChanges) {
        ctx.global().change(stat, change);

        if (!ctx.global().hidden(stat))
            ctx.sendGlobalStat(stat);
    }
}

RandomEngine dices_rd { now() };

uint dices(const uint dices, const uint max) {
    std::uniform_int_distribution<uint> dice { 1, max };

    uint result { 0 };
    for (uint i { 0 }; i < dices; i++)
        result += dice(dices_rd);

    return result;
}

std::optional<byte> votePlayer(Gameplay& interface, const std::string& msg, const byte target = ACTIVE_PLAYERS) {
    const OptionsList players_name { interface.names() };
    const std::optional<byte> player_number { vote(interface.ask(target, msg, players_name)) };

    if (!player_number)
        return {};

    const std::string& selected_name { players_name.at(*player_number) };
    const std::vector<byte> players_id { interface.players() };
    const auto selected_player = std::find_if(players_id.cbegin(), players_id.cend(), [&selected_name, &interface](const byte p_id) {
        return interface.player(p_id).name() == selected_name;
    });

    assert(selected_player != players_id.cend());
    return *selected_player;
}

}

void InstructionsProvider::initGameplayAPI() {
    sol::usertype<Gameplay> gameplay_type { ctx_.new_usertype<Gameplay>("Gameplay") };
    gameplay_type["global"] = &Gameplay::global;
    gameplay_type["game"] = &Gameplay::game;
    gameplay_type["rest"] = &Gameplay::rest;
    gameplay_type["checkpoint"] = &Gameplay::checkpoint;
    gameplay_type["player"] = &Gameplay::player;
    gameplay_type["players"] = &Gameplay::players;
    gameplay_type["activePlayers"] = &Gameplay::activePlayers;
    gameplay_type["names"] = &Gameplay::names;
    gameplay_type["count"] = &Gameplay::count;
    gameplay_type["leader"] = &Gameplay::leader;
    gameplay_type["switchLeader"] = &Gameplay::switchLeader;
    gameplay_type["voteForLeader"] = &Gameplay::voteForLeader;
    gameplay_type["ask"] = sol::overload(
            [](Gameplay& ctx, const byte target, const std::string& msg, const OptionsList& options) {
                return ctx.ask(target, msg, options);
            },
            [](Gameplay& ctx, const byte target, const std::string& msg, const OptionsList& options, const bool first_reply_only) {
                return ctx.ask(target, msg, options, first_reply_only);
            },
            &Gameplay::ask
    );
    gameplay_type["askNumber"] = sol::overload(
            [](Gameplay& ctx, const byte target, const std::string& msg, const byte min, const byte max) {
                return ctx.askNumber(target, msg, min, max);
            },
            [](Gameplay& ctx, const byte target, const std::string& msg, const byte min, const byte max, const bool first_reply_only) {
                return ctx.askNumber(target, msg, min, max, first_reply_only);
            },
            &Gameplay::askNumber
    );
    gameplay_type["askConfirm"] = sol::overload(
            [](Gameplay& ctx, const byte target) { return ctx.askConfirm(target); },
            &Gameplay::askConfirm
    );
    gameplay_type["askYesNo"] = sol::overload(
            [](Gameplay& ctx, const byte target, const std::string& question) {
                return ctx.askYesNo(target, question);
            },
            [](Gameplay& ctx, const byte target, const std::string& question, const bool first_reply_only) {
                return ctx.askYesNo(target, question, first_reply_only);
            },
            &Gameplay::askYesNo
    );
    gameplay_type["askDiceRoll"] = &Gameplay::askDiceRoll;
    gameplay_type["checkPlayer"] = &Gameplay::checkPlayer;
    gameplay_type["checkGame"] = &Gameplay::checkGame;
    gameplay_type["print"] = sol::overload(
            [](Gameplay& ctx, const std::string& text) { ctx.print(text); }, &Gameplay::print
    );
    gameplay_type["printImportant"] = sol::overload(
            [](Gameplay& ctx, const std::string& text) { ctx.printImportant(text); }, &Gameplay::printImportant
    );
    gameplay_type["printNote"] = sol::overload(
            [](Gameplay& ctx, const std::string& note) { ctx.printNote(note); }, &Gameplay::printNote
    );
    gameplay_type["printTitle"] = &Gameplay::printTitle;
    gameplay_type["sendGlobalStat"] = &Gameplay::sendGlobalStat;
    gameplay_type["sendPlayerUpdate"] = &Gameplay::sendPlayerUpdate;
    gameplay_type["sendBattleInit"] = &Gameplay::sendBattleInit;
    gameplay_type["sendBattleAtk"] = &Gameplay::sendBattleAtk;
    gameplay_type["sendBattleEnd"] = &Gameplay::sendBattleEnd;

    ctx_["getIDs"] = getIDs;
    ctx_["reply"] = reply;
    ctx_["vote"] = vote;
    ctx_["assertArgs"] = assertArgs;
    ctx_["toBoolean"] = toBoolean;
    ctx_["applyToGlobal"] = applyToGlobal;
    ctx_["dices"] = dices;
    ctx_["votePlayer"] = sol::overload(
        [](Gameplay& interface, const std::string& msg) { return votePlayer(interface, msg); },
        votePlayer
    );
    ctx_["ALL_PLAYERS"] = ALL_PLAYERS;
    ctx_["ACTIVE_PLAYERS"] = ACTIVE_PLAYERS;
    ctx_["YES"] = YES;
    ctx_["NO"] = NO;
    ctx_["setmetatable"] = sol::nil;
    ctx_["getmetatable"] = sol::nil;
#ifdef NDEBUG
    ctx_["print"] = sol::nil;
#endif
}

} // namespace Rbo::Server
