#ifndef ASIOCOMMON_HPP
#define ASIOCOMMON_HPP

#include "Common.hpp"

#include "boost/asio.hpp"

class Data;

namespace io = boost::asio;
using ErrCode = boost::system::error_code;
using tcp = io::ip::tcp;
using ReceiveBuffer = std::array<byte, 100>;

io::const_buffer trunc(const Data&);
ErrCode trySend(tcp::socket&, const io::const_buffer&);

#endif // ASIOCOMMON_HPP
