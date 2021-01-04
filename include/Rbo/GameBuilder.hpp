#ifndef GAMEBUILDER_HPP
#define GAMEBUILDER_HPP

#include <Rbo/Common.hpp>

namespace Rbo {

struct CheckpointLoadingError : std::exception {
    std::string msg;

    explicit CheckpointLoadingError(const std::string& err) : msg { err } {}

    virtual const char* what() const noexcept override { return msg.c_str(); };
};

struct GameState {
    word scene;
    Stats global;
    byte leader;
    PlayersState players;
};

class GameBuilder {
public:
    virtual ~GameBuilder() = default;

    virtual Game operator()() const = 0;
    virtual GameState load(const std::string& checkpt_final_name) const = 0;
    virtual std::string save(const std::string& checkpt_name, const GameState& state) const = 0;
    virtual Scene buildScene(const word id) const = 0;
};

} // namespace Rbo

#endif // GAMEBUILDER_HPP
