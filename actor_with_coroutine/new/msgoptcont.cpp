#include "msgoptcont.hpp"
#include "actor.hpp"

void MsgOptContPromiseFinalAwaiter::await_suspend(std::coroutine_handle<MsgOptContPromise> handle) noexcept
{
    handle.promise().continuation.resume();
}

MsgOptCont MsgOptContPromise::get_return_object() noexcept
{
    return {std::coroutine_handle<MsgOptContPromise>::from_promise(*this)};
}

void MsgOptContPromise::return_value(std::optional<Message> msg)
{
    actor->updateLastMsg(std::move(msg));
}

void MsgOptContAwaitable::await_suspend(std::coroutine_handle<> h) noexcept
{
    handle.promise().continuation = h;
}

std::optional<Message> MsgOptContAwaitable::await_resume() noexcept
{
    return handle.promise().actor->getLastMsg();
}
