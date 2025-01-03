//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/file.hpp"
#include "sl/io/result.hpp"

#include <sl/meta/enum/flag.hpp>
#include <sl/meta/func/unit.hpp>

#include <chrono>
#include <span>

#include <sys/epoll.h>

namespace sl::io {

class epoll {
    explicit epoll(int epfd) : handle_{ epfd } {}

public:
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

    [[nodiscard]] static result<epoll> create();

    [[nodiscard]] result<meta::unit> ctl(op an_op, const file& a_file, epoll_event an_event);
    [[nodiscard]] result<meta::unit> ctl(op an_op, file::view a_file, epoll_event an_event);
    [[nodiscard]] result<std::uint32_t>
        wait(std::span<epoll_event> out_events, tl::optional<std::chrono::milliseconds> maybe_timeout);

private:
    file handle_;
};

} // namespace sl::io
