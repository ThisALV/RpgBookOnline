#ifndef REPLYHANDLER_HPP
#define REPLYHANDLER_HPP

#include <Rbo/AsioCommon.hpp>

#include <memory>

namespace Rbo {

struct InvalidReply;

struct RequestProfile {
    tcp::socket* connection;
    bool targetted;
};

struct RequestCtx {
    std::map<byte, RequestProfile> players;
    Replies replies;
    std::atomic<ulong> repliesHandled;
    std::atomic<ulong> repliesToAccept;
    std::atomic_bool requestDone;
    std::vector<byte> errorIDs;

    RequestCtx() : repliesHandled { 0 }, repliesToAccept { 0 }, requestDone { false } {}

    RequestCtx(const RequestCtx&) = delete;
    RequestCtx& operator=(const RequestCtx&) = delete;

    bool operator==(const RequestCtx&) = delete;
};

using RequestCtxPtr = std::shared_ptr<RequestCtx>;

class ReplyHandler {
public:
    struct NetworkError : std::runtime_error {
        NetworkError(const std::string& operation, const ErrCode& err) : std::runtime_error { operation + " : " + err.category().name() + (" - " + err.message()) } {}
    };

    ReplyHandler(io::io_context::strand& handlersExecutor, spdlog::logger& sessionlogger, const RequestCtxPtr ctx, const ReplyController controller, const byte player);

    ReplyHandler(const ReplyHandler&) = delete;
    ReplyHandler& operator=(const ReplyHandler&) = delete;

    bool operator==(const ReplyHandler&) const = delete;

    ReplyHandler(ReplyHandler&& other) = default;
    ReplyHandler& operator=(ReplyHandler&& rhs) = default;

    void handle(const ErrCode error, const std::size_t);

private:
    io::io_context::strand* executor_;
    spdlog::logger* logger_;
    RequestCtxPtr ctx_;
    ReplyController controlValidity;
    byte playerID_;

    ReceiveBuffer replyBuffer_;

    void listenReply();
    void handleReply(const ErrCode error, const std::size_t replyLength);
    ReplyValidity treatReply(const std::size_t replyLength);

    void reportError(const byte player_id, const NetworkError& error) const;
    void handleError(const NetworkError& error) const;
};

}

#endif // REPLYHANDLER_HPP
