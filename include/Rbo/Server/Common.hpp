#ifndef SERVER_COMMONS_HPP
#define SERVER_COMMONS_HPP

#include <Rbo/Common.hpp>

namespace Rbo::Server {

struct NotAnyMaster : std::logic_error {
    NotAnyMaster() : std::logic_error { "There isn't any master member" } {}
};

struct GameBuildingError : std::runtime_error {
    explicit GameBuildingError(const std::string& msg) : std::runtime_error { msg } {}
};

struct Member {
    std::string name;
    bool ready;
};

using MembersStates = std::map<byte, Member>;
using Master = std::optional<byte>;

} // namespace Rbo::Server

#endif // SERVER_COMMONS_HPP
