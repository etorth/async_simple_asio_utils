#pragma once
#include <any>
#include <optional>
#include <coroutine>
#include <exception>
#include <cassert>
#include "raiitimer.hpp"

namespace corof
{
    struct base_promise
    {
        std::coroutine_handle<> handle = nullptr;

        base_promise *m_inner_promise = nullptr;
        base_promise *m_outer_promise = nullptr;
    };

    template<typename T> class [[nodiscard]] eval_poller
    {
        private:
            class eval_poller_promise;

        public:
            using promise_type = eval_poller_promise;

        private:
            class eval_poller_promise final: public base_promise
            {
                // hiden its definition and expose by aliasing to promise_type
                // this type is for compiler, user should never instantiate an eval_poller_promise object

                private:
                    friend class eval_poller;

                private:
                    T m_value;

                private:
                    std::exception_ptr m_exception = nullptr;

                public:
                    auto initial_suspend()
                    {
                        return awaiter(std::coroutine_handle<eval_poller_promise>::from_promise(*this));
                    }

                    auto final_suspend() noexcept
                    {
                        return std::suspend_always{};
                    }

                    void return_value(T t)
                    {
                        m_value = std::move(t);
                    }

                    eval_poller get_return_object()
                    {
                        return {std::coroutine_handle<eval_poller_promise>::from_promise(*this)};
                    }

                    void unhandled_exception()
                    {
                        m_exception = std::current_exception();
                    }

                    void rethrow_if_unhandled_exception()
                    {
                        if(m_exception){
                            std::rethrow_exception(m_exception);
                        }
                    }
            };

            class [[nodiscard]] awaiter
            {
                private:
                    friend class eval_poller;

                private:
                    std::coroutine_handle<eval_poller_promise> m_awaiter_handle;

                private:
                    explicit awaiter(std::coroutine_handle<eval_poller_promise> handle)
                        : m_awaiter_handle(handle)
                    {
                        m_awaiter_handle.promise().handle = m_awaiter_handle;
                    }

                public:
                    awaiter(awaiter && other)
                    {
                        std::swap(m_awaiter_handle, other.m_awaiter_handle);
                        assert(m_awaiter_handle);
                    }

                public:
                    awaiter              (const awaiter &) = delete;
                    awaiter & operator = (const awaiter &) = delete;

                public:
                    ~awaiter()
                    {
                        // if(m_awaiter_handle){
                        //     m_awaiter_handle.destroy();
                        // }
                    }

                public:
                    bool await_ready() noexcept
                    {
                        return false;
                    }

                public:
                    template<typename OUT_PROMISE> void await_suspend(std::coroutine_handle<OUT_PROMISE> handle) noexcept
                    {
                        /**/      handle.promise().m_inner_promise = std::addressof(m_awaiter_handle.promise());
                        m_awaiter_handle.promise().m_outer_promise = std::addressof(          handle.promise());
                    }

                public:
                    T await_resume()
                    {
                        return m_awaiter_handle.promise().m_value;
                    }
            };

        private:
            std::coroutine_handle<eval_poller_promise> m_handle;

        public:
            eval_poller(std::coroutine_handle<eval_poller_promise> handle = nullptr)
                : m_handle(handle)
            {}

        public:
            eval_poller(eval_poller && other) noexcept
            {
                std::swap(m_handle, other.m_handle);
            }

        public:
            eval_poller & operator = (eval_poller && other) noexcept
            {
                std::swap(m_handle, other.m_handle);
                return *this;
            }

        public:
            eval_poller              (const eval_poller &) = delete;
            eval_poller & operator = (const eval_poller &) = delete;

        public:
            ~eval_poller()
            {
                if(m_handle){
                    m_handle.destroy();
                }
            }

        public:
            bool valid() const
            {
                return m_handle.address();
            }

        public:
            bool poll()
            {
                assert(m_handle);
                auto curr_promise = find_promise(std::addressof(m_handle.promise()));

                if(curr_promise->handle.done()){
                    if(!curr_promise->m_outer_promise){
                        return true;
                    }

                    // jump out for one layer
                    // should I call destroy() for done handle?

                    auto outer_promise = curr_promise->m_outer_promise;

                    curr_promise = outer_promise;
                    curr_promise->m_inner_promise = nullptr;
                }

                // resume only once and return immediately
                // after resume curr_handle can be in done state, next call to poll should unlink it

                curr_promise->handle.resume();
                return m_handle.done();
            }

        private:
            static inline base_promise *find_promise(base_promise *promise)
            {
                auto curr_promise = promise;
                auto next_promise = promise->m_inner_promise;

                while(curr_promise && next_promise){
                    curr_promise = next_promise;
                    next_promise = next_promise->m_inner_promise;
                }
                return curr_promise;
            }

        public:
            decltype(auto) sync_eval()
            {
                while(!poll()){
                    continue;
                }
                return awaiter(m_handle).await_resume();
            }

        public:
            awaiter operator co_await()
            {
                return awaiter(m_handle);
            }
    };
}
