#ifndef LOCALGAMEBUILDER_HPP
#define LOCALGAMEBUILDER_HPP

#include <filesystem>
#include <random>
#include <Rbo/GameBuilder.hpp>
#include <Rbo/Server/InstructionsProvider.hpp>

namespace Rbo::Server {

namespace fs = std::filesystem;

struct ScriptLoadingError : std::runtime_error {
    ScriptLoadingError(const fs::path& script, const std::string& msg)
        : std::runtime_error { "Unable to load instructions \"" + script.string() + "\" : " + msg } {}
};

struct GameLoadingError : std::runtime_error {
    GameLoadingError(const std::string& msg) : std::runtime_error { "Unable to load game : " + msg } {}
};

struct GameSavingError : std::runtime_error {
    GameSavingError(const std::string& msg) : std::runtime_error { "Unable to save game : " + msg } {}
};

struct SceneLoadingError : std::runtime_error {
    SceneLoadingError(const word id, const std::string& msg) : std::runtime_error { "Unable to load scene " + std::to_string(id) + " : " + msg } {}
};

struct CheckpointAlreadyExists : std::logic_error {
    CheckpointAlreadyExists(const std::string& name) : std::logic_error { "Checkpoint \"" + name + "\" already" } {}
};

class LocalGameBuilder : public GameBuilder {
private:
    static RandomEngine chkpt_id_rd_;

    fs::path game_;
    fs::path chkpts_;
    fs::path scenes_;
    spdlog::logger& logger_;
    sol::state exec_ctx_;
    sol::table scenes_table_;
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
