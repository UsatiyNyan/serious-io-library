//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/sys/epoll.hpp"
#include "sl/io/sys/socket.hpp"

#include <sl/exec/model/concept.hpp>
#include <sl/exec/model/connection.hpp>
#include <sl/meta/traits/unique.hpp>

namespace sl::io::async {

struct server : meta::immovable {
    using V = std::pair<sys::socket, sys::address>;
    using E = std::error_code;

    using slot_callback = exec::slot_callback<std::pair<sys::socket, sys::address>, std::error_code>;

    template <exec::SlotCtor<V, E> SlotCtorT>
    struct [[nodiscard]] accept_connection final : slot_callback {
        accept_connection(server& self, SlotCtorT&& slot_ctor) : slot_{ std::move(slot_ctor)() }, self_{ self } {}

        // Connection
        exec::CancelHandle auto emit() && noexcept {
            callback_ = [this](sys::epoll::event_flag events) {
                if (events & sys::epoll::event::in) {
                    self_.accept_impl(callback_, *this);
                } else {
                    std::unreachable();
                };
            };
            self_.accept_impl(callback_, *this);
            return exec::proxy_cancel_handle{ this };
        }

        // CancelHandle
        void try_cancel() && noexcept {
            if (std::exchange(callback_, {})) {
                self_.try_cancel_impl(*this);
            }
        }

        void set_result(meta::maybe<meta::result<V, E>>&& maybe_result) && noexcept override {
            callback_ = {};
            exec::fulfill_slot(std::move(slot_), std::move(maybe_result));
        }

    private:
        sys::epoll::callback callback_;
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

public:
    [[nodiscard]] static result<std::unique_ptr<server>> create(sys::listening_server& a_sys, sys::epoll& an_epoll);

    constexpr exec::Signal<V, E> auto accept() & { return accept_signal{ *this }; }

private:
    explicit server(sys::listening_server& a_sys, sys::epoll& an_epoll) : sys_{ a_sys }, epoll_{ an_epoll } {}

    void accept_impl(sys::epoll::callback& epoll_cb, slot_callback& slot_cb) &;
    void try_cancel_impl(slot_callback& slot_cb) &;

private:
    sys::listening_server& sys_;
    sys::epoll& epoll_;
    bool registered_ = false;
};

} // namespace sl::io::async
