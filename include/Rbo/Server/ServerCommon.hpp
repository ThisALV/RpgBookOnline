#ifndef SERVER_COMMONS
#define SERVER_COMMONS

#include <Rbo/Common.hpp>

namespace Rbo::Server {

struct Member {
    std::string name;
    bool ready;
};

using MembersStates = std::map<byte, Member>;

} // namespace Rbo::Server

#endif // SERVER_COMMONS
