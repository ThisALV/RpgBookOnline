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
        NetworkError(const std::string& operation, const ErrCode& err) : std::runtime_error { operation + " : " + err.category().name() + (" - " + err.message()) } {}
    };

    ReplyHandler(io::io_context::strand& handlersExecutor, spdlog::logger& sessionlogger, RequestCtx& ctx, const ReplyController controller, const byte player);

    ReplyHandler(const ReplyHandler&) = delete;
    ReplyHandler& operator=(const ReplyHandler&) = delete;

    bool operator==(const ReplyHandler&) const = delete;

    ReplyHandler(ReplyHandler&& other) = default;
    ReplyHandler& operator=(ReplyHandler&& rhs) = default;

    void handle(const ErrCode error, const std::size_t);

private:
    io::io_context::strand* executor_;
    spdlog::logger* logger_;
    RequestCtx* ctx_;
    ReplyController controlValidity;
    byte playerID_;

    ReceiveBuffer replyBuffer_;

    void listenReply();
    void handleReply(const ErrCode error, const std::size_t replyLength);
    ReplyValidity treatReply(const std::size_t replyLength);

    void handleError(const NetworkError& error) const;
};

}

#endif // REPLYHANDLER_HPP
