#include "printmessage.hpp"
#include "actor.hpp"

void Actor::send(uint64_t toAddress, Message message, std::function<void(const Message&)> callback)
{
    auto target = pool.getActor(toAddress);
    if (target) {
        // If the callback is nullptr, set seqID to 0
        if (callback == nullptr) {
            message.seqID = 0;
        } else {
            // Store the callback in the hash table
            callbacks[message.seqID] = callback;
        }

        {
            std::unique_lock<std::mutex> lock(target->mailboxMutex);
            target->mailbox.push_back(message);
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

    // If the received message's seqID is non-zero, send a reply
    if (message.seqID != 0) {
        Message replyMessage("Reply from Actor!", address, 0, message.seqID);
        send(message.from, replyMessage);
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
        if (messages.empty()) break;
        for (auto& message : messages) {
            receive(message);
        }
    }
}

void Actor::sendMessages()
{
    constexpr int M = 20; // Number of actors
    int messageCount = std::rand() % 5 + 1;

    for (int i = 0; i < messageCount; ++i) {
        uint64_t randomActorAddress;
        do {
            randomActorAddress = std::rand() % M + 1;
        } while (randomActorAddress == getAddress());

        int sequenceNumber = sequence++;
        Message message("Hello from Actor!", address, sequenceNumber);

        // Randomly decide whether to provide a callback or not
        if (std::rand() % 2 == 0) {
            send(randomActorAddress, message, [this, sequenceNumber](const Message& reply)
            {
                printMessage("Actor %llu received reply for sequence number %d: %s\n", getAddress(), sequenceNumber, reply.content.c_str());
            });
        }
        else {
            send(randomActorAddress, message);
        }
    }
}
