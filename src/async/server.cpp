//
// Created by usatiynyan.
//

#include "sl/io/async/server.hpp"

#include <sl/exec/model/syntax.hpp>
#include <sl/meta/assert.hpp>

namespace sl::io::async {

result<std::unique_ptr<server>> server::create(sys::listening_server& a_sys, sys::epoll& an_epoll) {
    return a_sys //
        .get_socket()
        .set_blocking(false)
        .map([&](auto) { return std::unique_ptr<server>{ new server{ a_sys, an_epoll } }; });
}

void server::accept_impl(sys::epoll::callback& epoll_cb, slot_callback& slot_cb) & {
    auto accept_result = sys_.accept();
    if (!check_reschedule(accept_result)) {
        std::move(slot_cb).set_result(std::move(accept_result));
        return;
    }
    constexpr auto subscribe_events = sys::epoll::event_flag{} //
                                      | sys::epoll::event::in //
                                      | sys::epoll::event::et //
                                      | sys::epoll::event::oneshot;
    const auto op = registered_ ? sys::epoll::op::mod : sys::epoll::op::add;
    epoll_ //
        .ctl(op, sys_.get_socket().get_file(), subscribe_events, epoll_cb)
        .map([&](meta::unit) { registered_ = true; })
        .map_error([&](std::error_code ec) { std::move(slot_cb).set_result(meta::err(ec)); });
}

void server::try_cancel_impl(slot_callback& slot_cb) & {
    epoll_ //
        .ctl(sys::epoll::op::del, sys_.get_socket().get_file(), ::epoll_event{})
        .map([&](meta::unit) { std::move(slot_cb).set_result(meta::null); })
        .map_error([&](std::error_code ec) { std::move(slot_cb).set_result(meta::err(ec)); });
}

} // namespace sl::io::async
