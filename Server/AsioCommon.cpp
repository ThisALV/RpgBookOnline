#include "AsioCommon.hpp"

#include "Data.hpp"

io::const_buffer trunc(const Data& data) {
    return io::buffer(data.buffer(), data.count());
}

ErrCode trySend(tcp::socket& player, const io::const_buffer& buffer) {
    ErrCode err;
    player.send(buffer, {}, err);

    return err;
}
