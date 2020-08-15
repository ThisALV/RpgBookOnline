#include "AsioCommon.hpp"

#include "Data.hpp"

namespace Rbo {

io::const_buffer trunc(const Data& data) {
    return io::buffer(data.buffer(), data.count());
}

ErrCode trySend(tcp::socket& player, const io::const_buffer& buffer) {
    ErrCode err;
    player.send(buffer, {}, err);

    return err;
}

} // namespace Rbo
