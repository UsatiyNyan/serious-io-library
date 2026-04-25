//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/state/server.hpp"
#include "sl/io/sys/epoll.hpp"

#include <sl/exec/model/concept.hpp>
#include <sl/meta/traits/unique.hpp>

namespace sl::io::async {

struct server : meta::immovable {
    using V = std::pair<sys::socket, sys::address>;
    using E = std::error_code;

    template <exec::SlotCtor<V, E> SlotCtorT>
    struct [[nodiscard]] accept_connection final : state::server::callback {
        accept_connection(state::server& a_state, sys::epoll& an_epoll, SlotCtorT&& slot_ctor)
            : slot_{ std::move(slot_ctor)() }, state_{ a_state }, epoll_{ an_epoll } {}

        // Connection
        exec::CancelHandle auto emit() && noexcept {
            state_.begin_accept(*this);
            return exec::proxy_cancel_handle{ this };
        }

        // CancelHandle
        constexpr void try_cancel() && noexcept { state_.cancel_accept(); }

        // slot_callback
        void set_result(meta::maybe<meta::result<V, E>>&& maybe_result) && noexcept override {
            exec::fulfill_slot(std::move(slot_), std::move(maybe_result));
        }

    private:
        exec::SlotFrom<SlotCtorT> slot_;
        state::server& state_;
        sys::epoll& epoll_;
    };

    struct [[nodiscard]] accept_signal {
        using value_type = V;
        using error_type = E;

    public:
        accept_signal(state::server& a_state, sys::epoll& an_epoll) : state_{ a_state }, epoll_{ an_epoll } {}

        template <exec::SlotCtor<V, E> SlotCtorT>
        constexpr exec::Connection auto subscribe(SlotCtorT&& slot_ctor) && noexcept {
            return accept_connection{ state_, epoll_, std::move(slot_ctor) };
        }

        static exec::executor& get_executor() noexcept { return exec::inline_executor(); }

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
        constexpr exec::Signal<V, E> auto accept() & { return accept_signal{ state_, epoll_ }; }

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
