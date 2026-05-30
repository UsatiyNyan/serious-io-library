//
// Created by usatiynyan.
//

#include "sl/io/async/socket.hpp"

#include <sl/exec/model/syntax.hpp>
#include <sl/meta/assert.hpp>

namespace sl::io::async {

[[nodiscard]] result<std::unique_ptr<socket>> socket::create(sys::socket& a_sys, sys::epoll& an_epoll) {
    return a_sys //
        .set_blocking(false)
        .map([&](auto) { return std::unique_ptr<socket>{ new socket{ a_sys, an_epoll } }; });
}

void socket::error_impl(slot_callback& slot_cb) {
    const auto socket_error_result = sys_.get_opt(SOL_SOCKET, SO_ERROR);
    const auto an_error = socket_error_result //
                              .map([](auto x) { return std::make_error_code(std::errc{ x }); })
                              .value_or(socket_error_result.error());
    std::move(slot_cb).set_result(meta::err(an_error));
}

void socket::close_impl(slot_callback& slot_cb) {
    const auto an_error = std::make_error_code(std::errc::connection_reset);
    std::move(slot_cb).set_result(meta::err(an_error));
}

void socket::read_impl(sys::epoll::callback& epoll_cb, slot_callback& slot_cb, std::span<std::byte> buffer) & {
    const auto read_result = sys_.get_file().read(buffer);
    if (!check_reschedule(read_result)) {
        std::move(slot_cb).set_result(std::move(read_result));
        return;
    }
    constexpr auto subscribe_events = sys::epoll::event_flag{} //
                                      | sys::epoll::event::in //
                                      | sys::epoll::event::rdhup //
                                      | sys::epoll::event::hup //
                                      | sys::epoll::event::err //
                                      | sys::epoll::event::et //
                                      | sys::epoll::event::oneshot;
    const auto op = registered_ ? sys::epoll::op::mod : sys::epoll::op::add;
    epoll_ //
        .ctl(op, sys_.get_file(), subscribe_events, epoll_cb)
        .map([&](meta::unit) { registered_ = true; })
        .map_error([&](std::error_code ec) { std::move(slot_cb).set_result(meta::err(ec)); });
}

void socket::write_impl(sys::epoll::callback& epoll_cb, slot_callback& slot_cb, std::span<const std::byte> buffer) & {
    const auto write_result = sys_.get_file().write(buffer);
    if (!check_reschedule(write_result)) {
        std::move(slot_cb).set_result(std::move(write_result));
        return;
    }
    constexpr auto subscribe_events = sys::epoll::event_flag{} //
                                      | sys::epoll::event::out //
                                      | sys::epoll::event::rdhup //
                                      | sys::epoll::event::hup //
                                      | sys::epoll::event::err //
                                      | sys::epoll::event::et //
                                      | sys::epoll::event::oneshot;
    const auto op = registered_ ? sys::epoll::op::mod : sys::epoll::op::add;
    epoll_ //
        .ctl(op, sys_.get_file(), subscribe_events, epoll_cb)
        .map([&](meta::unit) { registered_ = true; })
        .map_error([&](std::error_code ec) { std::move(slot_cb).set_result(meta::err(ec)); });
}

void socket::try_cancel_impl(slot_callback& slot_cb) & {
    epoll_ //
        .ctl(sys::epoll::op::del, sys_.get_file(), ::epoll_event{})
        .map([&](meta::unit) { std::move(slot_cb).set_result(meta::null); })
        .map_error([&](std::error_code ec) { std::move(slot_cb).set_result(meta::err(ec)); });
}

} // namespace sl::io::async
