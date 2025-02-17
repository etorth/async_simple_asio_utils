#include "printmessage.hpp"
#include "actor.hpp"

MsgOptCont Actor::send(int toAddress, int respID, std::string message, bool waitResp)
{
    if(auto target = pool.getActor(toAddress); target){
        Message msg
        {
            .content = std::move(message),
            .from = getAddress(),
            .respID = respID,
        };

        if(waitResp){
            msg.seqID = sequence++;
        }

        {
            std::unique_lock<std::mutex> lock(target->mailboxMutex);
            target->mailbox.push_back(std::move(msg));
        }

        pool.scheduleActor(target);
        if(waitResp){
            co_return co_await SendMsgAwaitable{this, msg.seqID};
        }
        else{
            co_return std::nullopt;
        }
    }

    co_return Message
    {
        .content = "No actor found",
    };
}

void Actor::receive(const Message& msg)
{
    printMessage("Actor %llu received message from Actor %d with content: %s, seqID: %d, respID: %s\n", address, msg.from, msg.content.c_str(), msg.seqID, (msg.respID != 0 ? std::to_string(msg.respID).c_str() : "null"));
    updateLastMsg(msg);

    if(msg.respID > 0){
        onContMessage(msg);
    }
    else if(auto h = onFreeMessage(msg).handle; !h.done()){
        m_respHandlerList.emplace(msg.seqID, h);
    }
}

void Actor::consumeMessages()
{
    if(!doneInitCall){
        initCall();
        doneInitCall = true;
    }

    while (true) {
        std::vector<Message> messages;
        {
            std::unique_lock<std::mutex> lock(mailboxMutex);
            std::swap(messages, mailbox);
        }

        if (messages.empty()){
            break;
        }

        for (auto& message : messages) {
            receive(std::move(message));
        }
    }
}

MsgOptCont Actor::onFreeMessage(const Message &msg)
{
    updateLastMsg(msg);

    if(msg.seqID > 0){
        if(msg.content == "?"){
            auto askMsg = co_await send(msg.from, 0, "ask");
            co_return Message
            {
                .content = std::string("Reply ") + askMsg.value().content,
                .from = getAddress(),
                .respID = msg.seqID,
            };
        }
        else{
            co_return Message
            {
                .content = "Reply",
                .from = getAddress(),
                .respID = msg.seqID,
            };
        }
    }
    co_return std::nullopt;
}

void Actor::onContMessage(const Message &msg)
{
    if(auto p = m_respHandlerList.find(msg.respID); p != m_respHandlerList.end()){
        p->second.resume();
        m_respHandlerList.erase(p);
    }
}

void Actor::initCall()
{
    MsgOptContAwaitable(initCallCoroutine().handle).await_suspend(std::noop_coroutine());
}

MsgOptCont Actor::initCallCoroutine()
{
    for(int recvAddr: {getAddress() - 1, getAddress() + 1}){
        std::string message("Hello from Actor!");

        if (std::rand() % 2 == 0) {
            auto replyOpt = co_await send(recvAddr, 0, message, true);
            auto reply = replyOpt.value();
            printMessage("Actor %llu send: %s, seqID %d to %d, received reply from %d: %s\n", getAddress(), message.c_str(), reply.respID, recvAddr, reply.from, reply.content.c_str());
        }
        else {
            auto replyOpt = co_await send(recvAddr, 0, message);
            if(replyOpt.has_value()){
                printMessage("Actor %llu send: %s, to %d, received unexpected message: %s\n", getAddress(), message.c_str(), recvAddr, replyOpt.value().content.c_str());
            }
            else{
                printMessage("Actor %llu send: %s, to %d\n", getAddress(), message.c_str(), recvAddr);
            }
        }
    }
}
