#pragma once

#include <coroutine>
#include <exception>
#include <iostream>
#include <utility>

class [[nodiscard]] task;

struct task_promise
{
    struct final_awaitable
    {
        bool await_ready() const noexcept { return false; }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<task_promise> coro) noexcept
        {
            return coro.promise().m_continuation;
        }

        void await_resume() noexcept {}
    };


    task get_return_object() noexcept;
    std::suspend_always initial_suspend() const noexcept { return {}; }
    auto final_suspend() const noexcept { return final_awaitable(); }
    void return_void() noexcept {}
    void unhandled_exception() noexcept { exit(1); }

    void set_continuation(std::coroutine_handle<> continuation) noexcept
    {
        m_continuation = continuation;
    }

private:
    std::coroutine_handle<> m_continuation = std::noop_coroutine();
};

class [[nodiscard]] task
{
public:
    using promise_type = task_promise;

    explicit task(std::coroutine_handle<task_promise> handle)
        : m_handle(handle)
    {
    }

    ~task()
    {
        if (m_handle)
        {
            m_handle.destroy();
        }
    }

    auto operator co_await() noexcept
    {
        struct awaiter
        {
            bool await_ready() const noexcept { return !m_coro || m_coro.done(); }
            std::coroutine_handle<> await_suspend( std::coroutine_handle<> awaiting_coroutine) noexcept {
                m_coro.promise().set_continuation(awaiting_coroutine);
                return m_coro;
            }
            void await_resume() noexcept {}

            std::coroutine_handle<task_promise> m_coro;
        };
        return awaiter{m_handle};
    }

private:
    std::coroutine_handle<task_promise> m_handle;
};

inline task task_promise::get_return_object() noexcept
{
    return task{ std::coroutine_handle<task_promise>::from_promise(*this) };
}