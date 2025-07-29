//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/state/server.hpp"
#include "sl/io/sys/epoll.hpp"

#include <sl/exec/algo/sched/inline.hpp>
#include <sl/exec/model/concept.hpp>
#include <sl/meta/traits/unique.hpp>

namespace sl::io::async {

struct server : meta::immovable {
    struct accept_connection : exec::cancel_mixin {
        using value_type = std::pair<sys::socket, sys::address>;
        using error_type = std::error_code;

    public:
        accept_connection(state::server& a_state, sys::epoll& an_epoll, exec::slot<value_type, error_type>& slot)
            : state_{ a_state }, epoll_{ an_epoll }, slot_{ slot } {}

        // Connection
        exec::cancel_mixin& get_cancel_handle() & { return *this; }
        void emit() &&;

        // cancel_mixin
        bool try_cancel() & override;

    private:
        state::server& state_;
        sys::epoll& epoll_;
        exec::slot<value_type, error_type>& slot_;
    };

    struct accept_signal {
        using value_type = std::pair<sys::socket, sys::address>;
        using error_type = std::error_code;

    public:
        accept_signal(state::server& a_state, sys::epoll& an_epoll) : state_{ a_state }, epoll_{ an_epoll } {}

        exec::executor& get_executor() & { return exec::inline_executor(); }
        exec::Connection auto subscribe(exec::slot<value_type, error_type>& slot) {
            return accept_connection{ state_, epoll_, slot };
        }

    private:
        state::server& state_;
        sys::epoll& epoll_;
    };

    struct [[nodiscard]] bound : meta::immovable {
        bound(state::server& a_state, sys::epoll& an_epoll) : state_{ a_state }, epoll_{ an_epoll } {}
        friend struct server;

    public:
        bound(bound&& other)
            : state_{ other.state_ }, epoll_{ other.epoll_ }, is_unbound_{ std::exchange(other.is_unbound_, true) } {}
        ~bound();

    public:
        exec::Signal<std::pair<sys::socket, sys::address>, std::error_code> auto accept() & {
            return accept_signal{ state_, epoll_ };
        }

        // expected to always be called in the end
        result<meta::unit> unbind() &&;

    private:
        state::server& state_;
        sys::epoll& epoll_;
        bool is_unbound_ = false;
    };

private:
    explicit server(state::server& a_server);

public:
    [[nodiscard]] static result<std::unique_ptr<server>> create(state::server& a_server);

    result<bound> bind(sys::epoll& an_epoll) &;

private:
    state::server& state_;
    sys::epoll::callback callback_;
};

} // namespace sl::io::async
