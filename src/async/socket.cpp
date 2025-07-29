//
// Created by usatiynyan.
//

#include "sl/io/async/socket.hpp"

#include <sl/exec/model/syntax.hpp>

#include <libassert/assert.hpp>

namespace sl::io::async {

void socket::read_connection::emit() && {
    state_.begin_read(buffer_, [&slot = slot_](result<value_type> a_result) {
        exec::fulfill_slot(slot, meta::maybe{ a_result });
    });
}

bool socket::read_connection::try_cancel() & { return state_.cancel_read(); }

void socket::write_connection::emit() && {
    state_.begin_write(buffer_, [&slot = slot_](result<value_type> a_result) {
        exec::fulfill_slot(slot, meta::maybe{ a_result });
    });
}

bool socket::write_connection::try_cancel() & { return state_.cancel_write(); }

socket::bound::~bound() { ASSERT(is_unbound_); }

result<meta::unit> socket::bound::unbind() && {
    ASSERT(!is_unbound_);
    return epoll_ //
        .ctl(sys::epoll::op::del, state_.sys().get_file(), ::epoll_event{})
        .map([this](meta::unit) {
            is_unbound_ = true;
            return meta::unit{};
        });
}

socket::socket(state::socket& a_socket)
    : state_{ a_socket }, callback_{ [this](sys::epoll::event_flag events) {
          if (events & sys::epoll::event::err) {
              state_.handle_error();
          } else if ((events & sys::epoll::event::rdhup) || (events & sys::epoll::event::hup)) {
              state_.handle_close();
          } else if (events & sys::epoll::event::in) {
              state_.resume_read();
          } else if (events & sys::epoll::event::out) {
              state_.resume_write();
          } else {
              UNREACHABLE(events);
          }
      } } {}

result<std::unique_ptr<socket>> socket::create(state::socket& a_socket) {
    return a_socket //
        .sys()
        .set_blocking(false)
        .map([&a_socket](auto) { return std::unique_ptr<socket>{ new socket{ a_socket } }; });
}

result<socket::bound> socket::bind(sys::epoll& an_epoll) & {
    constexpr auto subscribe_events = sys::epoll::event_flag{} //
                                      | sys::epoll::event::in //
                                      | sys::epoll::event::out //
                                      | sys::epoll::event::rdhup //
                                      | sys::epoll::event::hup //
                                      | sys::epoll::event::err //
                                      | sys::epoll::event::et;
    return an_epoll //
        .ctl(sys::epoll::op::add, state_.sys().get_file(), subscribe_events, callback_)
        .map([&](meta::unit) { return bound{ state_, an_epoll }; });
}

} // namespace sl::io::async
