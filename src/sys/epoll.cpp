//
// Created by usatiynyan.
//

#include "sl/io/sys/epoll.hpp"

#include <sl/meta/assert.hpp>

namespace sl::io::sys {

void epoll::callback::operator()(event_flag events) & {
    if (state_.load(std::memory_order::acquire) == state_destroyed) {
        delete this;
        return;
    }
    DEBUG_ASSERT(state_.load(std::memory_order::relaxed) == state_deferred);
    cb_(events);
}

void epoll::callback::try_destroy() & {
    state expected = state_deferred;
    if (state_.compare_exchange_strong(
            expected, state_destroyed, std::memory_order::relaxed, std::memory_order::acquire
        )) {
        return;
    }
    DEBUG_ASSERT(expected == state_eager);
    delete this;
}

void epoll::callback::eager() & {
    DEBUG_ASSERT(state_.load(std::memory_order::relaxed) != state_destroyed);
    state_.store(state_eager, std::memory_order::release);
}

void epoll::callback::defer() & {
    DEBUG_ASSERT(state_.load(std::memory_order::relaxed) == state_eager);
    state_.store(state_deferred, std::memory_order::release);
}

result<epoll> epoll::create() {
    const int epfd = epoll_create1(0);
    if (epfd == -1) {
        return meta::errno_err();
    }
    return epoll{ /* .handle = */ file{ epfd } };
}

result<meta::unit> epoll::ctl(op an_op, const file& a_file, ::epoll_event an_event) & {
    const int result = ::epoll_ctl(handle_.internal(), static_cast<int>(an_op), a_file.internal(), &an_event);
    if (result == -1) {
        return meta::errno_err();
    }
    return meta::unit{};
}

result<meta::unit> epoll::ctl(op an_op, const file& a_file, event_flag subscribe_events, callback* a_callback) & {
    return ctl(an_op, a_file, ::epoll_event{ .events = subscribe_events.get_underlying(), .data{ .ptr = a_callback } });
}

result<std::uint32_t>
    epoll::wait(std::span<epoll_event> out_events, meta::maybe<std::chrono::milliseconds> maybe_timeout) & {
    ASSERT(out_events.size() > 0);
    const int timeout_ms = maybe_timeout.map([](auto x) { return static_cast<int>(x.count()); }).value_or(-1);
    const int nfds = ::epoll_wait( //
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

void epoll::fulfill(std::span<::epoll_event> out_events) {
    for (const ::epoll_event& out_event : out_events) {
        auto* a_callback = static_cast<callback*>(out_event.data.ptr);
        ASSERT(a_callback != nullptr);
        (*a_callback)(epoll::event_flag{ out_event.events });
    }
}

} // namespace sl::io::sys
