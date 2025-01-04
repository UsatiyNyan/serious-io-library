//
// Created by usatiynyan.
//

#include "sl/io/sys/epoll.hpp"

#include <function2/function2.hpp>
#include <libassert/assert.hpp>

namespace sl::io {

result<epoll> epoll::create() {
    const int epfd = epoll_create1(0);
    if (epfd == -1) {
        return meta::errno_err();
    }
    return epoll{ epfd };
}

result<meta::unit> epoll::ctl(op an_op, const file& a_file, epoll_event an_event) {
    return ctl(an_op, file::view{ a_file }, an_event);
}

result<meta::unit> epoll::ctl(op an_op, file::view a_file, epoll_event an_event) {
    const int result = epoll_ctl(handle_.internal(), static_cast<int>(an_op), a_file.internal(), &an_event);
    if (result == -1) {
        return meta::errno_err();
    }
    return meta::unit{};
}

result<std::uint32_t>
    epoll::wait(std::span<epoll_event> out_events, tl::optional<std::chrono::milliseconds> maybe_timeout) {
    ASSERT(out_events.size() > 0);
    const int timeout_ms = maybe_timeout //
                               .map([](std::chrono::milliseconds x) { return static_cast<int>(x.count()); })
                               .value_or(-1);
    const int nfds = epoll_wait( //
        handle_.internal(),
        out_events.data(),
        static_cast<int>(out_events.size()),
        timeout_ms
    );
    if (nfds == -1) {
        return meta::errno_err();
    }
    ASSERT(nfds > 0);
    return static_cast<std::uint32_t>(nfds);
}

} // namespace sl::io
