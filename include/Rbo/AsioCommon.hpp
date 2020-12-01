#ifndef ASIOCOMMON_HPP
#define ASIOCOMMON_HPP

#include <Rbo/Common.hpp>

#include <boost/asio.hpp>

namespace Rbo {

class Data;

namespace io = boost::asio;
using ErrCode = boost::system::error_code;
using tcp = io::ip::tcp;
using ReceiveBuffer = std::array<byte, 100>;

io::const_buffer trunc(const Data& data);
ErrCode trySend(tcp::socket& connection, const io::const_buffer& buffer);

} // namespace Rbo

#endif // ASIOCOMMON_HPP
