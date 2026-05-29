//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/state/socket.hpp"
#include "sl/io/sys/epoll.hpp"

#include <sl/exec/model/concept.hpp>
#include <sl/exec/model/connection.hpp>
#include <sl/meta/traits/unique.hpp>

namespace sl::io::async {

struct socket : meta::immovable {
    using V = std::uint32_t;
    using E = std::error_code;

    template <exec::SlotCtor<V, E> SlotCtorT>
    struct [[nodiscard]] read_connection final : state::socket::slot_callback {
        read_connection(socket& self, std::span<std::byte> buffer, SlotCtorT&& slot_ctor)
            : slot_{ std::move(slot_ctor)() }, buffer_{ buffer }, self_{ self } {}

        // Connection
        exec::CancelHandle auto emit() && noexcept {
            return self_.state_.begin_read(buffer_, state::socket::callbacks{ .epoll = self_.callback_, .slot = this });
        }

        // slot_callback
        void set_result(meta::maybe<meta::result<V, E>>&& maybe_result) && noexcept override {
            exec::fulfill_slot(std::move(slot_), std::move(maybe_result));
        }

    private:
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
    struct [[nodiscard]] write_connection final : state::socket::slot_callback {
        write_connection(socket& self, std::span<const std::byte> buffer, SlotCtorT&& slot_ctor)
            : slot_{ std::move(slot_ctor)() }, buffer_{ buffer }, self_{ self } {}

        // Connection
        exec::CancelHandle auto emit() && noexcept {
            return self_.state_.begin_write(
                buffer_, state::socket::callbacks{ .epoll = self_.callback_, .slot = this }
            );
        }

        // slot_callback
        void set_result(meta::maybe<meta::result<V, E>>&& maybe_result) && noexcept override {
            exec::fulfill_slot(std::move(slot_), std::move(maybe_result));
        }

    private:
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

    struct [[nodiscard]] bound : meta::immovable {
        bound(socket& self, sys::epoll& an_epoll) : self_{ self }, epoll_{ an_epoll } {}
        friend struct socket;

    public:
        bound(bound&& other) noexcept
            : self_{ other.self_ }, epoll_{ other.epoll_ }, is_unbound_{ std::exchange(other.is_unbound_, true) } {}
        ~bound() noexcept;

    public:
        exec::Signal<V, E> auto read(std::span<std::byte> buffer) & { return read_signal{ self_, buffer }; }
        exec::Signal<V, E> auto write(std::span<const std::byte> buffer) & { return write_signal{ self_, buffer }; }

        // expected to always be called in the end
        result<meta::unit> unbind() &&;

    private:
        socket& self_;
        sys::epoll& epoll_;
        bool is_unbound_ = false;
    };

private:
    explicit socket(state::socket& a_socket);

public:
    socket(socket&& other) noexcept : state_{ other.state_ }, callback_{ std::exchange(other.callback_, nullptr) } {}
    ~socket() noexcept { callback_->try_destroy(); }

    [[nodiscard]] static result<std::unique_ptr<socket>> create(state::socket& a_socket);

    result<bound> bind(sys::epoll& an_epoll) &;

private:
    state::socket& state_;
    sys::epoll::callback* callback_;
};

} // namespace sl::io::async
