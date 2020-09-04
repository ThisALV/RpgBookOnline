#ifndef REPLYHANDLER_HPP
#define REPLYHANDLER_HPP

#include <Rbo/AsioCommon.hpp>

namespace Rbo {

enum struct ReplyValidity : byte;

struct InvalidReply;

struct RequestCtx {
    std::map<byte, tcp::socket*> players;
    Replies replies;
    std::atomic<ulong> counter;
    std::atomic<ulong> limit;
    std::vector<byte> errorIDs;

    RequestCtx() = default;

    RequestCtx(const RequestCtx&) = delete;
    RequestCtx& operator=(const RequestCtx&) = delete;

    bool operator==(const RequestCtx&) = delete;
};

class ReplyHandler {
public:
    struct NetworkError : std::runtime_error {
        NetworkError(const std::string&, const ErrCode&);
    };

    ReplyHandler(io::io_context::strand&, spdlog::logger&, RequestCtx&, const ReplyController,
                 const byte);

    ReplyHandler(const ReplyHandler&) = delete;
    ReplyHandler& operator=(const ReplyHandler&) = delete;

    bool operator==(const ReplyHandler&) const = delete;

    ReplyHandler(ReplyHandler&&) = default;
    ReplyHandler& operator=(ReplyHandler&&) = default;

    void handle(const ErrCode, const std::size_t);

private:
    io::io_context::strand* executor_;
    spdlog::logger* logger_;
    RequestCtx* ctx_;
    ReplyController controlValidity;
    byte playerID_;

    ReceiveBuffer replyBuffer_;

    void listenReply();
    void handleReply(const ErrCode, const std::size_t);
    ReplyValidity treatReply(const std::size_t);

    void handleError(const NetworkError&) const;
};

}

#endif // REPLYHANDLER_HPP
