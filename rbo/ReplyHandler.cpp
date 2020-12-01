#include <Rbo/ReplyHandler.hpp>

#include <spdlog/logger.h>
#include <Rbo/SessionDataFactory.hpp>

namespace Rbo {

ReplyHandler::ReplyHandler(io::io_context::strand& exectuor, spdlog::logger& logger, RequestCtx& ctx, const ReplyController controller, const byte p_id)
    : executor_ { &exectuor },
      logger_ { &logger },
      ctx_ { &ctx },
      controlValidity { controller },
      playerID_ { p_id } {}

void ReplyHandler::handleError(const NetworkError& err) const {
    logger_->error("[ReplyHandler {}] {}", playerID_, err.what());
    ctx_->errorIDs.push_back(playerID_);
    ctx_->counter++;
}

ReplyValidity ReplyHandler::treatReply(const std::size_t length) {
    byte reply;
    try {
        if (length != 1)
            throw InvalidReply { ReplyValidity::InavlidLengthError };

        reply = replyBuffer_[0];

        controlValidity(reply);
    } catch (const InvalidReply& err) {
        return err.type;
    }

    if (ctx_->replies.size() >= ctx_->limit) {
        logger_->info("Reply of [{}] ignored (too late).", playerID_);
        return ReplyValidity::TooLate;
    }

    SessionDataFactory anwser;
    anwser.makeReply(playerID_, reply);
    const io::const_buffer anwser_buffer { trunc(anwser.dataWithLength()) };

    for (auto [remote_id, remote_player] : ctx_->players) {
        if (ctx_->replies.count(remote_id) == 0)
            continue;

        const ErrCode err { trySend(*remote_player, anwser_buffer) };

        if (err)
            throw NetworkError { "send_answer" + std::to_string(remote_id), err };
    }

    logger_->info("Reply of [{}] : {}", playerID_, reply);
    ctx_->replies.insert({ playerID_, reply });

    return ReplyValidity::Ok;
}

void ReplyHandler::handleReply(const ErrCode r_err, const std::size_t length) {
    logger_->debug("Handling reply for [{}].", playerID_);
    try {
        if (r_err)
            throw NetworkError { "receive_reply", r_err };

        const ReplyValidity reply_validity { treatReply(length) };

        SessionDataFactory validity_data;
        validity_data.makeValidation(reply_validity);

        const ErrCode validation_err { trySend(*ctx_->players.at(playerID_), trunc(validity_data.dataWithLength())) };

        if (validation_err)
            throw NetworkError { "send_validation", validation_err };

        if (isInvalid(reply_validity)) {
            logger_->error("Invalid reply for [{}] : error code #{}.", playerID_, reply_validity);
            listenReply();
        } else {
            for (const auto [id, reply] : ctx_->replies) {
                SessionDataFactory reply_data;
                reply_data.makeReply(id, reply);

                const ErrCode err { trySend(*ctx_->players.at(playerID_), trunc(reply_data.dataWithLength())) };

                if (err)
                    throw NetworkError { "receive_answers" + std::to_string(id), err };
            }

            logger_->info("Reply of {} is valid.", playerID_);
            ctx_->counter++;
        }
    } catch (const NetworkError& err) {
        handleError(err);
    }
}

void ReplyHandler::listenReply() {
    logger_->debug("Listening reply for [{}]...", playerID_);
    ctx_->players.at(playerID_)->async_receive(
                io::buffer(replyBuffer_), io::bind_executor(*executor_, std::bind(&ReplyHandler::handleReply, this, std::placeholders::_1, std::placeholders::_2)));
}

void ReplyHandler::handle(const ErrCode send_err, const std::size_t) {
    if (send_err) {
        handleError({ "request:send", send_err });
        return;
    }

    logger_->debug("Request sent to {}.", playerID_);
    replyBuffer_.fill(0);
    listenReply();
}

} // namespace Rbo
