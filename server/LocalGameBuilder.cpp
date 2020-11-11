#include <Rbo/Server/LocalGameBuilder.hpp>

#include <fstream>
#include <spdlog/logger.h>
#include <spdlog/fmt/ostr.h>
#include <Rbo/GameJsonCast.inl>

namespace Rbo::Server {

RandomEngine LocalGameBuilder::chkpt_id_rd_ { now() };

LocalGameBuilder::LocalGameBuilder(const fs::path& game, const fs::path& chkpts, const fs::path& scenes, const fs::path& scripts_dir)
    : game_ { game }, chkpts_ { chkpts }, scenes_ { scenes },
      logger_ { rboLogger("GameBuilder") }, exec_ctx_ {}, provider_ { exec_ctx_, logger_ }
{
    logger_.info("Loading scenes...");
    try {
        scenes_table_ = exec_ctx_.script_file(scenes_.string()).get<sol::table>();
    } catch (const sol::error& err) {
        throw GameLoadingError { err.what() };
    }
    logger_.info("Scenes loaded.");

    logger_.info("Reading instructions...");
    if (!fs::is_directory(scripts_dir))
        throw std::invalid_argument { "Given script directory isn't a directory" };

    const fs::directory_iterator scripts { scripts_dir, fs::directory_options::skip_permission_denied };

    for (const fs::directory_entry& entry : scripts) {
        if (!fs::is_regular_file(entry) || entry.path().extension() != ".lua") {
            logger_.debug("{} ignored.", entry.path());
            continue;
        }

        logger_.debug("Executing {}...", entry.path());
        try {
            exec_ctx_.script_file(entry.path().string());
        } catch (const sol::error& err) {
            throw ScriptLoadingError { entry, err.what() };
        }
    }
    logger_.info("Instructions read.");

    logger_.info("Loading read instructions...");
    provider_.load();
    logger_.info("Instructions loaded.");
}

Game LocalGameBuilder::operator()() const {
    logger_.info("Loading game {}...", game_);

    Game game;
    try {
        std::ifstream in { game_ };
        json data;
        in >> data;

        if (in.fail())
            throw GameLoadingError { "Error on input stream" };

        data.at("name").get_to(game.name);
        data.at("globals").get_to(game.globalStats);
        data.at("players").get_to(game.playerStats);
        data.at("inventories").get_to(game.playerInventories);
        data.at("bonuses").get_to(game.bonuses);
        data.at("effects").get_to(game.eventEffects);
        data.at("enemies").get_to(game.enemies);
        data.at("groups").get_to(game.groups);
        data.at("rest").get_to(game.rest);
        data.at("playerDeath").get_to(game.deathConditions);
        data.at("gameEnd").get_to(game.gameEndConditions);
        data.at("voteOnLeaderDeath").get_to(game.voteOnLeaderDeath);
        data.at("voteLeader").get_to(game.voteLeader);

        for (const Game::Error err : game.validity())
            logger_.warn("Spotted errors in game : {}", Game::getMessage(err));
    } catch (const json::exception& err) {
        throw GameLoadingError { err.what() };
    }

    logger_.info("Game loaded.");
    return game;
}

GameState LocalGameBuilder::load(const std::string& name) const {
    logger_.info("Opening checkpoints in {} to find \"{}\"...", chkpts_, name);

    GameState state;
    try {
        std::ifstream in { chkpts_ };
        json data;
        in >> data;

        if (in.fail())
            throw GameLoadingError { "Error on input stream" };

        const json& chkpt { data.at(name) };
        chkpt.at("scene").get_to(state.scene);
        chkpt.at("global").get_to(state.global);
        chkpt.at("leader").get_to(state.leader);
        chkpt.at("players").get_to(state.players);
    } catch (const json::exception& err) {
        throw GameLoadingError { err.what() };
    }

    logger_.info("Searched checkpoint read.");
    return state;
}

std::string LocalGameBuilder::save(const std::string& name, const GameState& state) const {
    const std::string final_name { name + '_' + std::to_string(std::uniform_int_distribution { 0, 5000 } (chkpt_id_rd_)) };

    logger_.info("Opening checkpoints in {} to write \"{}\" under the name of \"{}\"...", chkpts_, name, final_name);
    json data;
    try {
        std::ifstream in { chkpts_ };
        in >> data;

        if (in.fail())
            throw GameLoadingError { "Error on input stream" };
    } catch (const json::exception& err) {
        throw GameLoadingError { err.what() };
    }

    json& chkpt { data[final_name] };
    if (chkpt.is_object())
        throw CheckpointAlreadyExists { final_name };

    try {
        std::ofstream out { chkpts_ };

        chkpt = json::object();
        chkpt["scene"] = state.scene;
        chkpt["global"] = state.global;
        chkpt["leader"] = state.leader;
        chkpt["players"] = state.players;

        out << data.dump(4);
        if (out.fail())
            throw GameSavingError { "Error in input stream" };
    } catch (const json::exception& err) {
        throw GameSavingError { err.what() };
    }

    logger_.info("Checkpoint saved.");
    return final_name;
}

Scene LocalGameBuilder::buildScene(const word id) const {
    logger_.info("Building scene {}...", id);

    Scene scene;
    const sol::object scene_obj { scenes_table_[id].get<sol::object>() };
    if (scene_obj.get_type() != sol::type::table)
        throw SceneLoadingError { id, "Unknown scene ID" };

    for (const auto instruction_entry : scene_obj.as<sol::table>()) {
        if (instruction_entry.second.get_type() != sol::type::table)
            throw SceneLoadingError { id, "One of this scene instructions isn't a Lua table" };

        const sol::table instruction { instruction_entry.second.as<sol::table>() };
        const sol::object name { instruction[1].get<sol::object>() };
        const sol::object args { instruction[2].get<sol::object>() };

        if (name.get_type() != sol::type::string || args.get_type() != sol::type::table)
            throw SceneLoadingError { id, "Invalid instruction : [1] != string or [2] != table" };

        scene.push_back(provider_.get(name.as<std::string>(), args.as<sol::table>()));
    }

    logger_.info("Scene built.");
    return scene;
}

} // namespace Rbo::Server
