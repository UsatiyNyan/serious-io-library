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
    struct [[nodiscard]] accept_connection final : state::server::slot_callback {
        accept_connection(server& a_state, SlotCtorT&& slot_ctor) : slot_{ std::move(slot_ctor)() }, self_{ a_state } {}

        // Connection
        exec::CancelHandle auto emit() && noexcept {
            return self_.state_.begin_accept(state::server::callbacks{ .epoll = self_.callback_, .slot = this });
        }

        // slot_callback
        void set_result(meta::maybe<meta::result<V, E>>&& maybe_result) && noexcept override {
            exec::fulfill_slot(std::move(slot_), std::move(maybe_result));
        }

    private:
        exec::SlotFrom<SlotCtorT> slot_;
        server& self_;
    };

    struct [[nodiscard]] accept_signal {
        using value_type = V;
        using error_type = E;

    public:
        constexpr explicit accept_signal(server& self) : self_{ self } {}

        template <exec::SlotCtor<V, E> SlotCtorT>
        constexpr exec::Connection auto subscribe(SlotCtorT&& slot_ctor) && noexcept {
            return accept_connection{ self_, std::move(slot_ctor) };
        }

        static exec::executor& get_executor() noexcept { return exec::inline_executor(); }

    private:
        server& self_;
    };

    struct [[nodiscard]] bound : meta::immovable {
        bound(server& self, sys::epoll& an_epoll) : self_{ self }, epoll_{ an_epoll } {}
        friend struct server;

    public:
        bound(bound&& other) noexcept
            : self_{ other.self_ }, epoll_{ other.epoll_ }, is_unbound_{ std::exchange(other.is_unbound_, true) } {}
        ~bound() noexcept;

    public:
        constexpr exec::Signal<V, E> auto accept() & { return accept_signal{ self_ }; }

        // expected to always be called in the end
        result<meta::unit> unbind() &&;

    private:
        server& self_;
        sys::epoll& epoll_;
        bool is_unbound_ = false;
    };

private:
    explicit server(state::server& a_server);

public:
    server(server&& other) noexcept : state_{ other.state_ }, callback_{ std::exchange(other.callback_, nullptr) } {}
    ~server() noexcept {
        if (callback_) {
            callback_->try_destroy();
        }
    }

    [[nodiscard]] static result<std::unique_ptr<server>> create(state::server& a_server);

    result<bound> bind(sys::epoll& an_epoll) &;

private:
    state::server& state_;
    sys::epoll::callback* callback_;
};

} // namespace sl::io::async
