#ifndef LOCALGAMEBUILDER_HPP
#define LOCALGAMEBUILDER_HPP

#include <filesystem>
#include <random>
#include "GameBuilder.hpp"
#include "InstructionsProvider.hpp"

namespace Rbo::Server {

namespace fs = std::filesystem;

struct ScriptLoadingError : std::runtime_error {
    ScriptLoadingError(const fs::path& script, const std::string& msg)
        : std::runtime_error { "Impossible de charger les instructions dans \"" + script.string() + "\" : " + msg } {}
};

struct GameLoadingError : std::runtime_error {
    GameLoadingError(const std::string& msg)
        : std::runtime_error { "Impossible de charger le jeu : " + msg } {}
};

struct GameSavingError : std::runtime_error {
    GameSavingError(const std::string& msg)
        : std::runtime_error { "Impossible de sauvegarder le jeu : " + msg } {}
};

struct CheckpointAlreadyExists : std::logic_error {
    CheckpointAlreadyExists(const std::string& name)
        : std::logic_error { "Le checkpoint \"" + name + "\" existe déjà" } {}
};

struct SceneLoadingError : std::runtime_error {
    SceneLoadingError(const word id, const std::string& msg)
        : std::runtime_error {
              "Impossible de charger la scène " + std::to_string(id) + " : " + msg } {}
};

class LocalGameBuilder : public GameBuilder {
private:
    static std::default_random_engine chkpt_id_rd_;

    fs::path game_;
    fs::path chkpts_;
    fs::path scenes_;
    spdlog::logger& logger_;
    sol::state exec_ctx_;
    InstructionsProvider provider_;

public:
    LocalGameBuilder(const fs::path&, const fs::path&, const fs::path&, const fs::path&);
    virtual ~LocalGameBuilder() = default;

    LocalGameBuilder(const LocalGameBuilder&) = delete;
    LocalGameBuilder& operator=(const LocalGameBuilder&) = delete;

    bool operator==(const LocalGameBuilder&) const = delete;

    virtual Game operator()() const override;
    virtual GameState load(const std::string&) const override;
    virtual std::string save(const std::string&, const GameState&) const override;
    virtual Scene buildScene(const word) const override;
};

} // namespace Rbo::Server

#endif // LOCALGAMEBUILDER_HPP
