#pragma once

#include <coroutine>

#include "fire_once_event.hpp"
#include "msgoptcont.hpp"

struct sync_wait_task_promise;

struct [[nodiscard]] sync_wait_task
{
    using promise_type = sync_wait_task_promise;

    sync_wait_task(std::coroutine_handle<sync_wait_task_promise> coro)
        : m_handle(coro)
    {
    }

    ~sync_wait_task()
    {
        if (m_handle)
        {
            m_handle.destroy();
        }
    }

    void run(fire_once_event& event);
private:
    std::coroutine_handle<sync_wait_task_promise> m_handle;
};

struct sync_wait_task_promise
{
    std::suspend_always initial_suspend() const noexcept { return {}; }

    auto final_suspend() const noexcept
    {
        struct awaiter
        {
            bool await_ready() const noexcept { return false; }

            void await_suspend(std::coroutine_handle<sync_wait_task_promise> coro) const noexcept
            {
                fire_once_event *const event = coro.promise().m_event;
                if (event)
                {
                    event->set();
                }
            }

            void await_resume() noexcept {}
        };
        return awaiter();
    }

    fire_once_event *m_event = nullptr;

    sync_wait_task get_return_object() noexcept
    {
        return sync_wait_task{ std::coroutine_handle<sync_wait_task_promise>::from_promise(*this) };
    }

    void unhandled_exception() noexcept { exit(1); }
};


inline void sync_wait_task::run(fire_once_event& event)
{
    m_handle.promise().m_event = &event;
    m_handle.resume();
}

inline sync_wait_task make_sync_wait_task(MsgOptCont &t)
{
    co_await t;
}

inline void sync_wait(MsgOptCont &t)
{
    fire_once_event event;
    auto wait_task = make_sync_wait_task(t);
    wait_task.run(event);
    event.wait();
}
