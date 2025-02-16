#pragma once
#include <coroutine>
#include "message.hpp"

struct MsgOptCont
{
    struct promise_type
    {
        std::optional<Message> m_msg;

        MsgOptCont get_return_object() noexcept
        {
            return std::coroutine_handle<promise_type>::from_promise(*this);
        }

        std::suspend_always initial_suspend() const noexcept
        {
            return {};
        }

        std::suspend_always final_suspend() const noexcept
        {
            return {};
        }

        void return_value(std::optional<Message> msg)
        {
            m_msg = std::move(msg);
        }

        void unhandled_exception()
        {
            std::terminate();
        }
    };

    std::coroutine_handle<promise_type> m_coro;

    MsgOptCont(std::coroutine_handle<promise_type> coro)
        : m_coro(coro)
    {}

    struct MsgAwaitable
    {
        MsgOptCont* m_optCont;

        bool await_ready() const noexcept
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<>) noexcept
        {
        }

        std::optional<Message> await_resume() noexcept
        {
            return m_optCont->m_coro.promise().m_msg;
        }
    };

    MsgAwaitable operator co_await() noexcept
    {
        return {this};
    }
};
