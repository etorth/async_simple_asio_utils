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
        .content = "NO ACTOR FOUND",
    };
}

void Actor::receive(const Message& msg)
{
    printMessage("Actor %d receive: %s, from %d, seqID: %d, respID: %d\n", getAddress(), msg.content.c_str(), msg.from, msg.seqID, msg.respID);
    updateLastMsg(msg);

    if(msg.respID > 0){
        onContMessage(msg);
    }
    else{
        MsgOptContAwaitable(onFreeMessage(msg).handle).await_suspend(std::noop_coroutine());

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

MsgOptCont Actor::onFreeMessage(Message msg)
{
    updateLastMsg(msg);

    if(msg.seqID > 0){
        if(msg.content == "?"){
            auto askMsg = co_await send(msg.from, 0, "ASK BACK");
            co_await send(msg.from, msg.seqID, std::string("Reply ") + askMsg.value().content);
        }
        else{
            co_await send(msg.from, msg.seqID, "Reply");
        }
    }
    co_return std::nullopt;
}

void Actor::onContMessage(Message msg)
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

int Rand()
{
    static std::mutex lock;
    const std::lock_guard<std::mutex> lockGuard(lock);
    return std::rand();
}

MsgOptCont Actor::initCallCoroutine()
{
    for(int recvAddr: {getAddress() - 1, getAddress() + 1}){
        std::string message(Rand() % 2 ? "HELLO FROM ACTOR" : "?");

        if (Rand() % 2 == 0) {
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
