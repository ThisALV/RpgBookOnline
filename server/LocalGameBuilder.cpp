#include <Rbo/Server/LocalGameBuilder.hpp>

#include <fstream>
#include <spdlog/logger.h>
#include <spdlog/fmt/ostr.h>
#include <Rbo/Server/GameJsonCast.inl>

namespace Rbo::Server {

std::mt19937_64 LocalGameBuilder::chkpt_id_rd_ { now() };

LocalGameBuilder::LocalGameBuilder(const fs::path& game, const fs::path& chkpts,
                                   const fs::path& scenes, const fs::path& scripts_dir)
    : game_ { game }, chkpts_ { chkpts }, scenes_ { scenes },
      logger_ { rboLogger("GameBuilder") }, exec_ctx_ {}, provider_ { exec_ctx_, logger_ }
{
    logger_.trace("Chargement des scènes...");
    try {
        scenes_table_ = exec_ctx_.script_file(scenes_.string()).get<sol::table>();
    } catch (const sol::error& err) {
        throw GameLoadingError { err.what() };
    }

    if (!fs::is_directory(scripts_dir))
        throw std::invalid_argument { "Script dirs n'est pas un dossier" };

    const fs::directory_iterator scripts {
        scripts_dir, fs::directory_options::skip_permission_denied
    };

    for (const fs::directory_entry& entry : scripts) {
        if (!fs::is_regular_file(entry) || entry.path().extension() != ".lua") {
            logger_.trace("{} ignoré", entry.path());
            continue;
        }

        logger_.trace("Exécution de {}...", entry.path());
        try {
            exec_ctx_.script_file(entry.path().string());
        } catch (const sol::error& err) {
            throw ScriptLoadingError { entry, err.what() };
        }
    }

    provider_.load();
}

Game LocalGameBuilder::operator()() const {
    logger_.trace("Chargement du jeu {}...", game_);

    Game game;
    try {
        std::ifstream in { game_ };
        json data;
        in >> data;

        if (in.fail())
            throw GameLoadingError { "Erreur sur le flux d'entrée" };

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

        const Game::Validity validity { game.validity() };
        if (validity != Game::Validity::Ok)
            throw GameLoadingError { Game::getMessage(validity) };
    } catch (const json::exception& err) {
        throw GameLoadingError { err.what() };
    }

    logger_.trace("Chargement effectué.");
    return game;
}

GameState LocalGameBuilder::load(const std::string& name) const {
    logger_.trace("Chargement des checkpoints dans {}.", chkpts_);
    logger_.trace("Chargement de \"{}\".", name);

    GameState state;
    try {
        std::ifstream in { chkpts_ };
        json data;
        in >> data;

        if (in.fail())
            throw GameLoadingError { "Erreur sur le flux d'entrée" };

        const json& chkpt { data.at(name) };
        chkpt.at("scene").get_to(state.scene);
        chkpt.at("global").get_to(state.global);
        chkpt.at("leader").get_to(state.leader);
        chkpt.at("players").get_to(state.players);
    } catch (const json::exception& err) {
        throw GameLoadingError { err.what() };
    }

    return state;
}

std::string LocalGameBuilder::save(const std::string& name, const GameState& state) const {
    const std::string final_name { name + '_' +
                std::to_string(std::uniform_int_distribution { 0, 5000 } (chkpt_id_rd_)) };

    logger_.trace("Chargement des checkpoints dans {}.", chkpts_);
    json data;
    try {
        std::ifstream in { chkpts_ };
        in >> data;

        if (in.fail())
            throw GameLoadingError { "Erreur sur le flux d'entrée" };
    } catch (const json::exception& err) {
        throw GameLoadingError { err.what() };
    }

    json& chkpt { data[final_name] };
    if (chkpt.is_object())
        throw CheckpointAlreadyExists { final_name };

    logger_.trace("Sauvegarde sur {}.", final_name);

    try {
        std::ofstream out { chkpts_ };

        chkpt = json::object();
        chkpt["scene"] = state.scene;
        chkpt["global"] = state.global;
        chkpt["leader"] = state.leader;
        chkpt["players"] = state.players;

        out << data.dump(4);
        if (out.fail())
            throw GameSavingError { "Erreur sur le flux de sortie" };
    } catch (const json::exception& err) {
        throw GameSavingError { err.what() };
    }

    return final_name;
}

Scene LocalGameBuilder::buildScene(const word id) const {
    logger_.trace("Construction de la scène {}...", id);

    Scene scene;
    const sol::object scene_obj { scenes_table_[id].get<sol::object>() };
    if (scene_obj.get_type() != sol::type::table)
        throw SceneLoadingError { id, "Scène inexistante" };

    for (const auto instruction_entry : scene_obj.as<sol::table>()) {
        if (instruction_entry.second.get_type() != sol::type::table)
            throw SceneLoadingError { id,
                    "Entrée n'étant pas une table parmis les instructions" };

        const sol::table instruction { instruction_entry.second.as<sol::table>() };
        const sol::object name { instruction[1].get<sol::object>() };
        const sol::object args { instruction[2].get<sol::object>() };

        if (name.get_type() != sol::type::string || args.get_type() != sol::type::table)
            throw SceneLoadingError { id, "Instruction invalide : [1] != string ou [2] != table" };

        scene.push_back(provider_.get(name.as<std::string>(), args.as<sol::table>()));
    }

    logger_.trace("Construction effectuée.");
    return scene;
}

} // namespace Rbo::Server
