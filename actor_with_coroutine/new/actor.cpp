#include "printmessage.hpp"
#include "actor.hpp"

void Actor::send(int toAddress, int respID, std::string message, std::function<void(const Message&)> callback)
{
    if(auto target = pool.getActor(toAddress); target){
        Message msg
        {
            .content = std::move(message),
            .from = getAddress(),
            .respID = respID,
        };

        if(callback){
            msg.seqID = sequence++;
            callbacks[msg.seqID] = std::move(callback);
        }

        {
            std::unique_lock<std::mutex> lock(target->mailboxMutex);
            target->mailbox.push_back(std::move(msg));
        }

        pool.scheduleActor(target);
    }
}

void Actor::receive(const Message& message)
{
    printMessage("Actor %llu received message from Actor %d with content: %s, seqID: %d, respID: %s\n", address, message.from, message.content.c_str(), message.seqID, (message.respID != 0 ? std::to_string(message.respID).c_str() : "null"));

    m_lastMsg = message;
    m_msgCount++;

    if (message.respID != 0) {
        auto it = callbacks.find(message.respID);
        if (it != callbacks.end()) {
            it->second(m_lastMsg.value());
            callbacks.erase(it);
        } else {
            throw std::runtime_error("Callback not found for response ID: " + std::to_string(message.respID));
        }
    }

    if (message.seqID != 0) {
        send(message.from, message.seqID, "Reply from Actor!");
    }
}

void Actor::consumeMessages()
{
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
