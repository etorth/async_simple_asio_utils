#pragma once
#include <condition_variable>
#include <unordered_map>
#include <stdexcept>
#include <cstdio>
#include <cstdarg>
#include <atomic>

#include "message.hpp"
#include "threadpool.hpp"
#include "msgoptcont.hpp"

class Actor
{
    private:
        ThreadPool& pool;
        int address;
        std::vector<Message> mailbox;
        std::mutex mailboxMutex;
        std::atomic<bool> m_processing;
        std::unordered_map<int, std::function<void(const Message&)>> callbacks;
        std::unordered_map<int, std::coroutine_handle<MsgOptCont::promise_type>> m_respHandlerList;

        std::atomic<int> sequence{1}; // Start sequence from 1, as 0 will be used for messages that don't need a response

    private:
        bool doneInitCall = false;

    private:
        std::optional<Message> m_lastMsg {};
        size_t m_msgCount = 0;

    public:
        void updateLastMsg(std::optional<Message> msg)
        {
            m_lastMsg = std::move(msg);
            m_msgCount++;
        }

        std::optional<Message> getLastMsg() const
        {
            return m_lastMsg;
        }

    private:
        struct SendMsgAwaitable
        {
            Actor *actor;
            int seqID;

            bool await_ready() const
            {
                return false;
            }

            void await_suspend(std::coroutine_handle<MsgOptCont::promise_type> handle)
            {
                actor->m_respHandlerList.emplace(seqID, handle);
            }

            std::optional<Message> await_resume()
            {
                return actor->m_lastMsg;
            }
        };

    public:
        Actor(ThreadPool& pool, int address)
            : pool(pool)
            , address(address)
            , m_processing(false)
        {}

        int getAddress() const
        {
            return address;
        }

        bool trySetProcessing()
        {
            return !m_processing.exchange(true);
        }

        void resetProcessing()
        {
            m_processing = false;
        }

    public:
        MsgOptCont send(int, int, std::string, bool = false);

    public:
        void receive(const Message &);
        void consumeMessages();

    public:
        MsgOptCont onFreeMessage(const Message &);
        void       onContMessage(const Message &);

    public:
        void initCall();
        MsgOptCont initCallCoroutine();
};
