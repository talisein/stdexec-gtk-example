#include <iostream>
#include <coroutine>
#include <stdexec/execution.hpp>
#include <exec/task.hpp>
#include <glibmm.h>
#include <giomm.h>

namespace glib::coro
{
    struct task {
        struct promise_type
        {
            auto get_return_object()
            {
                return task(std::coroutine_handle<promise_type>::from_promise(*this));
            }

            std::suspend_always initial_suspend() noexcept { return {}; }

            struct final_awaiter
            {
                bool await_ready() noexcept { return false; }
                void await_resume() noexcept {}
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept
                {
                    // final_awaiter::await_suspend is called when the execution of the
                    // current coroutine (referred to by 'h') is about to finish.
                    // If the current coroutine was resumed by another coroutine via
                    // co_await get_task(), a handle to that coroutine has been stored
                    // as h.promise().previous. In that case, return the handle to resume
                    // the previous coroutine.
                    // Otherwise, return noop_coroutine(), whose resumption does nothing.

                    if (auto previous = h.promise().previous; previous)
                        return previous;
                    else
                        return std::noop_coroutine();
                }
            };

            final_awaiter final_suspend() noexcept { return {}; }
            void unhandled_exception() { throw; }

            void return_value(Glib::RefPtr<Gio::AsyncResult> r) { result = r; }
            Glib::RefPtr<Gio::AsyncResult> result;
            std::coroutine_handle<> previous;
        };

    task(std::coroutine_handle<promise_type> h) : coro(h) {};
        task(task&& t) = delete;
        ~task() { coro.destroy(); }


        struct awaiter
        {
            bool await_ready() { return false; }
            Glib::RefPtr<Gio::AsyncResult> await_resume() { return std::move(coro.promise().result); }
            auto await_suspend(std::coroutine_handle<> h)
            {
                coro.promise().previous = h;
                return coro;
            }
            std::coroutine_handle<promise_type> coro;
        };

        awaiter operator co_await() { return awaiter{coro}; }

        Glib::RefPtr<Gio::AsyncResult> operator()()
        {
            coro.resume();
            return std::move(coro.promise().result);
        }

    private:
    std::coroutine_handle<promise_type> coro;
    };

    task output_stream_write_all_coro(Glib::RefPtr<Gio::OutputStream> os, const void * buffer, gsize count)
    {
        Glib::RefPtr<Glib::MainLoop> coro_loop = Glib::MainLoop::create(Glib::MainContext::get_default(), false);
        Glib::RefPtr<Gio::AsyncResult> res;
        os->write_all_async(buffer, count, [&](Glib::RefPtr<Gio::AsyncResult> r) { res = r; coro_loop->quit(); });
        coro_loop->run();
        co_return res;
    }
}

namespace glib::execution
{

}
