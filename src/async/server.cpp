//
// Created by usatiynyan.
//

#include "sl/io/async/server.hpp"

#include <sl/exec/model/syntax.hpp>

#include <libassert/assert.hpp>

namespace sl::io::async {

void server::accept_connection::emit() && {
    state_.begin_accept([&slot = slot_](result<value_type> a_result) {
        exec::fulfill_slot(slot, meta::maybe{ std::move(a_result) });
    });
}

bool server::accept_connection::try_cancel() & { return state_.cancel_accept(); }

server::bound::~bound() { ASSERT(is_unbound_); }

result<meta::unit> server::bound::unbind() && {
    ASSERT(!is_unbound_);
    return epoll_ //
        .ctl(sys::epoll::op::del, state_.sys().get_socket().get_file(), ::epoll_event{})
        .map([this](meta::unit) {
            is_unbound_ = true;
            return meta::unit{};
        });
}

server::server(state::server& a_server)
    : state_{ a_server }, callback_{ [this](sys::epoll::event_flag events) {
          if (events & sys::epoll::event::in) {
              state_.resume_accept();
          } else {
              UNREACHABLE(events);
          }
      } } {}

result<std::unique_ptr<server>> server::create(state::server& a_server) {
    return a_server //
        .sys()
        .get_socket()
        .set_blocking(false)
        .map([&a_server](auto) { return std::unique_ptr<server>{ new server{ a_server } }; });
}

result<server::bound> server::bind(sys::epoll& an_epoll) & {
    constexpr auto subscribe_events = sys::epoll::event_flag{} //
                                      | sys::epoll::event::in //
                                      | sys::epoll::event::et;
    return an_epoll //
        .ctl(sys::epoll::op::add, state_.sys().get_socket().get_file(), subscribe_events, callback_)
        .map([&](meta::unit) { return bound{ state_, an_epoll }; });
}

} // namespace sl::io::async
