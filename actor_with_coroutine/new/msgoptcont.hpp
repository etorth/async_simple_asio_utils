#pragma once
#include <coroutine>
#include <optional>
#include "message.hpp"

class Actor;
struct MsgOptCont;

struct MsgOptContPromise;
struct MsgOptContPromiseFinalAwaiter
{
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<MsgOptContPromise>) noexcept;
    void await_resume() noexcept {}
};

struct MsgOptContPromise
{
    Actor *actor;
    std::coroutine_handle<> continuation;

    // template<typename... Args> MsgOptContPromise(Actor *self, Args && ...)
    //     : actor(self)
    // {}

    MsgOptCont get_return_object() noexcept;
    void return_value(std::optional<Message>);

    std::suspend_always         initial_suspend() const noexcept { return {}; }
    MsgOptContPromiseFinalAwaiter final_suspend() const noexcept { return {}; }


    void unhandled_exception()
    {
        std::terminate();
    }
};

struct MsgOptContAwaitable
{
    std::coroutine_handle<MsgOptContPromise> handle;

    bool await_ready() const noexcept
    {
        return false;
    }

    void await_suspend(std::coroutine_handle<> h) noexcept;
    std::optional<Message> await_resume() noexcept;
};

struct MsgOptCont
{
    using promise_type = MsgOptContPromise;
    std::coroutine_handle<promise_type> handle;

    MsgOptCont(std::coroutine_handle<promise_type> h)
        : handle(h)
    {}

    MsgOptContAwaitable operator co_await() noexcept
    {
        return {handle};
    }
};
