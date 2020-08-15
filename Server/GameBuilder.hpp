#ifndef GAMEBUILDER_HPP
#define GAMEBUILDER_HPP

#include "Common.hpp"

namespace Rbo {

const word INTRO { 0 };

struct CheckpointLoadingError : std::exception {
    std::string msg;

    CheckpointLoadingError(const std::string& err) : msg { err } {}

    virtual const char* what() const noexcept override { return msg.c_str(); };
};

struct GameState {
    word scene;
    Stats global;
    byte leader;
    PlayersStates players;
};

struct GameBuilder {
    virtual ~GameBuilder() = default;

    virtual Game operator()() const = 0;
    virtual GameState load(const std::string&) const = 0;
    virtual std::string save(const std::string&, const GameState&) const = 0;
    virtual Scene buildScene(const word) const = 0;
};

} // namespace Rbo

#endif // GAMEBUILDER_HPP
