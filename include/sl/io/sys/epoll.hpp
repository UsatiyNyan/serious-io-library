//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/result.hpp"
#include "sl/io/sys/file.hpp"

#include <sl/exec/thread/detail/atomic.hpp>
#include <sl/meta/enum/flag.hpp>
#include <sl/meta/func/function.hpp>
#include <sl/meta/monad/maybe.hpp>
#include <sl/meta/type/unit.hpp>

#include <chrono>
#include <span>

#include <sys/epoll.h>

namespace sl::io::sys {

struct epoll {
    enum class op : int {
        add = EPOLL_CTL_ADD,
        mod = EPOLL_CTL_MOD,
        del = EPOLL_CTL_DEL,
    };

    enum class event : std::uint32_t {
        in = EPOLLIN,
        out = EPOLLOUT,
        rdhup = EPOLLRDHUP,
        pri = EPOLLPRI,
        err = EPOLLERR,
        hup = EPOLLHUP,
        et = EPOLLET,
        oneshot = EPOLLONESHOT,
        wakeup = EPOLLWAKEUP,
        exclusive = EPOLLEXCLUSIVE,
    };
    using event_flag = meta::enum_flag<event>;

    struct callback {
        enum state : std::uint32_t {
            state_eager,
            state_deferred,
            state_destroyed,
        };

    public:
        static callback* make(meta::unique_function<void(event_flag events)> cb) {
            return new callback{ std::move(cb) };
        }

        void operator()(event_flag events) &;
        void try_destroy() &;
        void eager() &;
        void defer() &;

    private:
        explicit callback(meta::unique_function<void(event_flag events)> cb) : cb_{ std::move(cb) } {}

    private:
        meta::unique_function<void(event_flag events)> cb_;
        exec::detail::atomic<state> state_{ state_eager };
    };

private:
    explicit epoll(file handle) : handle_{ std::move(handle) } {}

public:
    [[nodiscard]] static result<epoll> create();

    [[nodiscard]] result<meta::unit> ctl(op an_op, const file& a_file, ::epoll_event an_event) &;
    [[nodiscard]] result<meta::unit>
        ctl(op an_op, const file& a_file, event_flag subscribe_events, callback* a_callback) &;
    [[nodiscard]] result<std::uint32_t>
        wait(std::span<::epoll_event> out_events, meta::maybe<std::chrono::milliseconds> maybe_timeout) &;

    static void fulfill(std::span<::epoll_event> out_events);

    result<std::uint32_t>
        wait_and_fulfill(std::span<::epoll_event> out_events, meta::maybe<std::chrono::milliseconds> maybe_timeout) {
        return wait(out_events, maybe_timeout).map([out_events](std::uint32_t nevents) {
            fulfill(out_events.subspan(0, nevents));
            return nevents;
        });
    }

private:
    file handle_;
};

} // namespace sl::io::sys
