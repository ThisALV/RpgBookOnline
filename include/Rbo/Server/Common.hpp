#ifndef SERVER_COMMONS_HPP
#define SERVER_COMMONS_HPP

#include <Rbo/Common.hpp>

#include <atomic>

namespace Rbo::Server {

struct NotAnyMaster : std::logic_error {
    NotAnyMaster() : std::logic_error { "There isn't any master member" } {}
};

struct GameBuildingError : std::runtime_error {
    explicit GameBuildingError(const std::string& msg) : std::runtime_error { msg } {}
};

class Master {
private:
    std::atomic_bool exists_;
    std::atomic<byte> id_;

public:
    Master() : exists_ { false } {}

    void emptyLobby() { exists_ = false; }
    bool exists() const { return exists_; }

    void set(const byte id);
    Master& operator=(const byte id);

    byte load() const;
    operator byte () const { return load(); }
    std::optional<byte> get() const { return exists() ? std::optional<byte> { id_ } : std::optional<byte> {}; }
};

struct Member {
    std::string name;
    bool ready;
};

using MembersStates = std::map<byte, Member>;

} // namespace Rbo::Server

#endif // SERVER_COMMONS_HPP