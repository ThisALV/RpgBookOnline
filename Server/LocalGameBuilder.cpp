#include "LocalGameBuilder.hpp"

#include <fstream>
#include "spdlog/logger.h"
#include "spdlog/fmt/ostr.h"
#include "GameJsonCast.inl"

namespace Rbo::Server {

LocalGameBuilder::LocalGameBuilder(const fs::path& game, const fs::path& chkpts,
                                   const fs::path& scenes, const fs::path& scripts_dir)
    : game_ { game }, chkpts_ { chkpts }, scenes_ { scenes },
      logger_ { rboLogger("GameBuilder") }
{
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine, sol::lib::string,
                       sol::lib::math, sol::lib::table);
    lua.create_named_table("Rbo");

    if (!fs::is_directory(scripts_dir))
        throw std::invalid_argument { "Script dirs n'est pas un dossier" };

    const fs::directory_iterator scripts {
        scripts_dir, fs::directory_options::skip_permission_denied
    };

    for (const fs::directory_entry& script : scripts) {
        logger_.trace("Exécution de {}...", script.path());

        try {
            lua.script_file(script.path());
        } catch (const sol::error& err) {
            throw ScriptLoadingError { script, err.what() };
        }
    }

    provider_ = InstructionsProvider { lua, logger_ };
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

GameState LocalGameBuilder::load(const std::string&) const {
    throw std::runtime_error { "Load : non implémentée" };
}

std::string LocalGameBuilder::save(const std::string&, const GameState&) const {
    throw std::runtime_error { "Save : non implémentée" };
}

Scene LocalGameBuilder::buildScene(const word id) const {
    logger_.trace("Construction de la scène {}...", id);

    Scene scene;
    try {
        std::ifstream in { scenes_ };
        json scenes;
        in >> scenes;

        if (in.fail())
            throw SceneLoadingError { id, "Erreur sur le flux d'entrée" };

        for (const json& instruction : scenes.at(std::to_string(id))) {
            const json& name { instruction.at("cmd") };
            const json& args { instruction.at("args") };

            scene.push_back(provider_.get(name.get<std::string>(),
                                          args.get<std::vector<std::string>>()));
        }
    } catch (const json::exception& err) {
        throw GameLoadingError { err.what() };
    }

    logger_.trace("Construction effectuée.");
    return scene;
}

} // namespace Rbo::Server
