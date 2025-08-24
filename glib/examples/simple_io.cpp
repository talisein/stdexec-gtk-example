#include <functional>
#include <iostream>
#include "glibmm.h"
#include "giomm.h"
#include "../execution.h"

struct before_state
{
    before_state() :
        done(false),
        count(0),
        cancellable (Gio::Cancellable::create()),
        buf(1'000'000, 'M')
    {
        std::tie(file, ios) = Gio::File::create_tmp();
        std::cout << "BEFORE: Created " << file->get_path() << '\n';
        file->remove();

        Glib::signal_timeout().connect_seconds([this, count = int{0}]() mutable -> bool {
            ++count;
            std::cout << "BEFORE: Still alive " << count << "\n";
            if (count == 2) {
                cancellable->cancel();
            }
            return true;
        }, 1);
    }

    bool done;
    int count;
    Glib::RefPtr<Gio::File> file;
    Glib::RefPtr<Gio::FileIOStream> ios;
    Glib::RefPtr<Gio::OutputStream> os;
    Glib::RefPtr<Gio::Cancellable> cancellable;
    const std::string buf;

    void on_written(Glib::RefPtr<Gio::AsyncResult> res) {

        gsize bytes_written = 0;
        try {
            (void) os->write_all_finish(res, bytes_written);
            if (count++ < 10000 && !cancellable->is_cancelled()) {
                os->write_all_async(buf.data(), buf.size(), std::bind_front(&before_state::on_written, this), cancellable);
            } else {
                if (cancellable->is_cancelled()) {
                    std::cerr << "BEFORE: Successful write even tho cancelled\n";
                } else {
                    std::cout << "BEFORE: Finished writing everything! " << count << "\n";
                }

                auto on_close = [this](auto res) {
                    os->close_finish(res);
                    std::cout << "BEFORE: Closed\n";
                    done = true;
                };
                os->close_async(on_close);
            }
        } catch (Gio::Error &e) {
            std::cerr << "BEFORE: Got error in write_all: " << e.what() << "\n";
            done = true;
        }
    }
};

struct with_coroutine_state
{
    bool done;
};

void before_stdexec(before_state &state)
{
    state.os = state.ios->get_output_stream();
    state.os->write_all_async(state.buf.data(), state.buf.size(), std::bind_front(&before_state::on_written, &state), state.cancellable);
}

void with_coroutine(with_coroutine_state &state)
{
    auto [file, ios] = Gio::File::create_tmp();
    std::cout << "CORO: Created " << file->get_path() << '\n';
    file->remove();
    auto os = ios->get_output_stream();
    std::string buf(1'000'000, 'C');

    for (int i = 0; i < 10000; ++i) {
        auto write_task = co_await glib::coro::output_stream_write_all_coro(os, buf.data(), buf.size());
//        auto async_res = write_task();
        gsize written;
        os->write_all_finish(write_task, written);
    }
    std::cout << "CORO: done\n";
    state.done = true;
}

int main()
{
    Glib::init();
    Gio::init();
    Glib::RefPtr<Glib::MainLoop> loop { Glib::MainLoop::create() };
    Glib::RefPtr<Glib::MainContext> ctx = loop->get_context();
    before_state before{};
    with_coroutine_state with_coroutine_state;

    ctx->invoke([&] { before_stdexec(before); return false; });
    ctx->invoke([&] { with_coroutine(with_coroutine_state); return false; });

    // Quit when every state is done
    Glib::signal_timeout().connect_seconds([&]() {
        if (before.done && with_coroutine_state.done)
            loop->quit();
        return true;
    }, 1);
    loop->run();

    return 0;
}
