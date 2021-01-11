#ifndef LOCALGAMEBUILDER_HPP
#define LOCALGAMEBUILDER_HPP

#include <Rbo/Server/ServerCommon.hpp>

#include <filesystem>
#include <Rbo/GameBuilder.hpp>
#include <Rbo/Server/InstructionsProvider.hpp>

namespace Rbo::Server {

namespace fs = std::filesystem;

struct ScriptLoadingError : GameBuildingError {
    ScriptLoadingError(const fs::path& script, const std::string& msg)
        : GameBuildingError { "Unable to load instructions \"" + script.string() + "\" : " + msg } {}
};

struct GameLoadingError : GameBuildingError {
    explicit GameLoadingError(const std::string& msg) : GameBuildingError { "Unable to load game : " + msg } {}
};

struct GameSavingError : std::runtime_error {
    explicit GameSavingError(const std::string& msg) : std::runtime_error { "Unable to save game : " + msg } {}
};

struct SceneLoadingError : std::runtime_error {
    SceneLoadingError(const word id, const std::string& msg) : std::runtime_error { "Unable to load scene " + std::to_string(id) + " : " + msg } {}
};

struct CheckpointAlreadyExists : std::logic_error {
    explicit CheckpointAlreadyExists(const std::string& name) : std::logic_error { "Checkpoint \"" + name + "\" already exists" } {}
};

class LocalGameBuilder : public GameBuilder {
private:
    static std::size_t counter_;

    const fs::path game_;
    const fs::path chkpts_;
    const fs::path scenes_;

    spdlog::logger& logger_;
    sol::state exec_ctx_;
    sol::table scenes_table_;
    InstructionsProvider provider_;

public:
    LocalGameBuilder(fs::path game_file, fs::path checkpts_file, fs::path scenes_file, const fs::path& instructions_dir);
    ~LocalGameBuilder() override = default;

    LocalGameBuilder(const LocalGameBuilder&) = delete;
    LocalGameBuilder& operator=(const LocalGameBuilder&) = delete;

    bool operator==(const LocalGameBuilder&) const = delete;

    Game operator()() const override;
    GameState load(const std::string& checkpt_final_name) const override;
    std::string save(const std::string& checkpt_generic_name, const GameState& state) const override;
    Scene buildScene(const word scene_id) const override;
};

} // namespace Rbo::Server

#endif // LOCALGAMEBUILDER_HPP
