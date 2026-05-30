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

struct socket : meta::immovable {
    using V = std::uint32_t;
    using E = std::error_code;

    using slot_callback = exec::slot_callback<V, E>;

    template <exec::SlotCtor<V, E> SlotCtorT>
    struct [[nodiscard]] read_connection final : slot_callback {
        read_connection(socket& self, std::span<std::byte> buffer, SlotCtorT&& slot_ctor)
            : slot_{ std::move(slot_ctor)() }, buffer_{ buffer }, self_{ self } {}

        // Connection
        exec::CancelHandle auto emit() && noexcept {
            callback_ = [this](sys::epoll::event_flag events) {
                if (events & sys::epoll::event::err) {
                    self_.error_impl(*this);
                } else if ((events & sys::epoll::event::rdhup) || (events & sys::epoll::event::hup)) {
                    self_.close_impl(*this);
                } else if (events & sys::epoll::event::in) {
                    self_.read_impl(callback_, *this, buffer_);
                } else {
                    std::unreachable();
                }
            };
            self_.read_impl(callback_, *this, buffer_);
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
        std::span<std::byte> buffer_;
        socket& self_;
    };

    struct [[nodiscard]] read_signal final {
        using value_type = V;
        using error_type = E;

    public:
        read_signal(socket& self, std::span<std::byte> buffer) : buffer_{ buffer }, self_{ self } {}

        template <exec::SlotCtor<V, E> SlotCtorT>
        constexpr exec::Connection auto subscribe(SlotCtorT&& slot_ctor) && noexcept {
            return read_connection{ self_, buffer_, std::move(slot_ctor) };
        }

        static exec::executor& get_executor() noexcept { return exec::inline_executor(); }

    private:
        std::span<std::byte> buffer_;
        socket& self_;
    };

    template <exec::SlotCtor<V, E> SlotCtorT>
    struct [[nodiscard]] write_connection final : slot_callback {
        write_connection(socket& self, std::span<const std::byte> buffer, SlotCtorT&& slot_ctor)
            : slot_{ std::move(slot_ctor)() }, buffer_{ buffer }, self_{ self } {}

        // Connection
        exec::CancelHandle auto emit() && noexcept {
            callback_ = [this](sys::epoll::event_flag events) {
                if (events & sys::epoll::event::err) {
                    self_.error_impl(*this);
                } else if ((events & sys::epoll::event::rdhup) || (events & sys::epoll::event::hup)) {
                    self_.close_impl(*this);
                } else if (events & sys::epoll::event::out) {
                    self_.write_impl(callback_, *this, buffer_);
                } else {
                    std::unreachable();
                }
            };
            self_.write_impl(callback_, *this, buffer_);
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
        std::span<const std::byte> buffer_;
        socket& self_;
    };

    struct [[nodiscard]] write_signal {
        using value_type = V;
        using error_type = E;

    public:
        write_signal(socket& self, std::span<const std::byte> buffer) : buffer_{ buffer }, self_{ self } {}

        template <exec::SlotCtor<V, E> SlotCtorT>
        constexpr exec::Connection auto subscribe(SlotCtorT&& slot_ctor) && noexcept {
            return write_connection{ self_, buffer_, std::move(slot_ctor) };
        }

        static exec::executor& get_executor() noexcept { return exec::inline_executor(); }

    private:
        std::span<const std::byte> buffer_;
        socket& self_;
    };

public:
    [[nodiscard]] static result<std::unique_ptr<socket>> create(sys::socket& a_sys, sys::epoll& an_epoll);

    exec::Signal<V, E> auto read(std::span<std::byte> buffer) & { return read_signal{ *this, buffer }; }
    exec::Signal<V, E> auto write(std::span<const std::byte> buffer) & { return write_signal{ *this, buffer }; }

private:
    explicit socket(sys::socket& a_sys, sys::epoll& an_epoll) : sys_{ a_sys }, epoll_{ an_epoll } {}

    void error_impl(slot_callback& slot_cb);
    void close_impl(slot_callback& slot_cb);
    void read_impl(sys::epoll::callback& epoll_cb, slot_callback& slot_cb, std::span<std::byte> buffer) &;
    void write_impl(sys::epoll::callback& epoll_cb, slot_callback& slot_cb, std::span<const std::byte> buffer) &;
    void try_cancel_impl(slot_callback& slot_cb) &;

private:
    sys::socket& sys_;
    sys::epoll& epoll_;
    bool registered_ = false;
};

} // namespace sl::io::async
