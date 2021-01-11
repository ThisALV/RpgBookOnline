#ifndef REPLYHANDLER_HPP
#define REPLYHANDLER_HPP

#include <Rbo/AsioCommon.hpp>

#include <memory>
#include <mutex>

namespace Rbo {

struct InvalidReply;

struct RequestProfile {
    tcp::socket* connection;
    bool targetted;
};

struct RequestCtx {
    std::mutex requestMtx;
    std::map<byte, RequestProfile> players;
    byte repliesToAccept;

    Replies replies;
    std::atomic<byte> repliesHandled;
    bool requestDone;
    std::vector<byte> errorIDs;

    RequestCtx() : repliesToAccept { 0 }, repliesHandled { 0 }, requestDone { false } {}

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

    ReplyHandler(spdlog::logger& logger, const RequestCtxPtr ctx, const ReplyController controller, const byte p_id);

    ReplyHandler(const ReplyHandler&) = delete;
    ReplyHandler& operator=(const ReplyHandler&) = delete;

    bool operator==(const ReplyHandler&) const = delete;

    ReplyHandler(ReplyHandler&& other) = default;
    ReplyHandler& operator=(ReplyHandler&& rhs) = default;

    void handle(const ErrCode error, const std::size_t);

private:
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
