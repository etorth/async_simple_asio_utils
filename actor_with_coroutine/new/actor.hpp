#pragma once
#include <condition_variable>
#include <unordered_map>
#include <stdexcept>
#include <cstdio>
#include <cstdarg>
#include <atomic>

#include "message.hpp"
#include "threadpool.hpp"

class Actor
{
    private:
        ThreadPool& pool;
        int address;
        std::vector<Message> mailbox;
        std::mutex mailboxMutex;
        std::atomic<bool> isProcessing;
        std::unordered_map<int, std::function<void(const Message&)>> callbacks;

        std::optional<Message> m_lastMsg;
        size_t m_msgCount;
        std::atomic<int> sequence{1}; // Start sequence from 1, as 0 will be used for messages that don't need a response

    public:
        Actor(ThreadPool& pool, int address)
            : pool(pool)
            , address(address)
            , isProcessing(false)
            , m_msgCount(0)
        {}

        int getAddress() const
        {
            return address;
        }

        bool trySetProcessing()
        {
            return !isProcessing.exchange(true);
        }

        void resetProcessing()
        {
            isProcessing = false;
        }

    public:
        void send(int, int, std::string, std::function<void(const Message &)> = nullptr);
        void receive(const Message &);
        void consumeMessages();
};
