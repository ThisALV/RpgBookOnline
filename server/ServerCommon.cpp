#include <Rbo/Server/ServerCommon.hpp>

namespace Rbo::Server {

void Master::set(const byte id) {
    exists_ = true;
    id_ = id;
}

Master& Master::operator=(const byte id) {
    set(id);

    return *this;
}

byte Master::load() const {
    if (!exists_)
        throw NotAnyMaster {};

    return id_;
}

} // namespace Rbo::Server
