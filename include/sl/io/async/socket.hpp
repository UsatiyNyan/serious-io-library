//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/state/socket.hpp"
#include "sl/io/sys/epoll.hpp"

#include <sl/exec/algo/sched/inline.hpp>
#include <sl/exec/model/concept.hpp>
#include <sl/meta/traits/unique.hpp>

namespace sl::io::async {

struct socket : meta::immovable {
    struct read_connection : exec::cancel_mixin {
        using value_type = std::uint32_t;
        using error_type = std::error_code;

    public:
        read_connection(
            state::socket& a_state,
            sys::epoll& an_epoll,
            std::span<std::byte> buffer,
            exec::slot<value_type, error_type>& slot
        )
            : buffer_{ buffer }, slot_{ slot }, state_{ a_state }, epoll_{ an_epoll } {}

        // Connection
        exec::cancel_mixin& get_cancel_handle() & { return *this; }
        void emit() &&;

        // cancel_mixin
        bool try_cancel() & override;

    private:
        std::span<std::byte> buffer_;
        exec::slot<value_type, error_type>& slot_;
        state::socket& state_;
        sys::epoll& epoll_;
    };

    struct [[nodiscard]] read_signal {
        using value_type = std::uint32_t;
        using error_type = std::error_code;

    public:
        read_signal(state::socket& a_state, sys::epoll& an_epoll, std::span<std::byte> buffer)
            : buffer_{ buffer }, state_{ a_state }, epoll_{ an_epoll } {}

        exec::executor& get_executor() & { return exec::inline_executor(); }
        exec::Connection auto subscribe(exec::slot<value_type, error_type>& slot) {
            return read_connection{ state_, epoll_, buffer_, slot };
        }

    private:
        std::span<std::byte> buffer_;
        state::socket& state_;
        sys::epoll& epoll_;
    };

    struct write_connection : exec::cancel_mixin {
        using value_type = std::uint32_t;
        using error_type = std::error_code;

    public:
        write_connection(
            state::socket& a_state,
            sys::epoll& an_epoll,
            std::span<const std::byte> buffer,
            exec::slot<value_type, error_type>& slot
        )
            : buffer_{ buffer }, slot_{ slot }, state_{ a_state }, epoll_{ an_epoll } {}

        // Connection
        exec::cancel_mixin& get_cancel_handle() & { return *this; }
        void emit() &&;

        // cancel_mixin
        bool try_cancel() & override;

    private:
        std::span<const std::byte> buffer_;
        exec::slot<value_type, error_type>& slot_;
        state::socket& state_;
        sys::epoll& epoll_;
    };

    struct [[nodiscard]] write_signal {
        using value_type = std::uint32_t;
        using error_type = std::error_code;

    public:
        write_signal(state::socket& a_state, sys::epoll& an_epoll, std::span<const std::byte> buffer)
            : buffer_{ buffer }, state_{ a_state }, epoll_{ an_epoll } {}

        exec::executor& get_executor() & { return exec::inline_executor(); }
        exec::Connection auto subscribe(exec::slot<value_type, error_type>& slot) {
            return write_connection{ state_, epoll_, buffer_, slot };
        }

    private:
        std::span<const std::byte> buffer_;
        state::socket& state_;
        sys::epoll& epoll_;
    };

    struct [[nodiscard]] bound : meta::immovable {
        bound(state::socket& a_state, sys::epoll& an_epoll) : state_{ a_state }, epoll_{ an_epoll } {}
        friend struct socket;

    public:
        bound(bound&& other)
            : state_{ other.state_ }, epoll_{ other.epoll_ }, is_unbound_{ std::exchange(other.is_unbound_, true) } {}
        ~bound();

    public:
        exec::Signal<std::uint32_t, std::error_code> auto read(std::span<std::byte> buffer) & {
            return read_signal{ state_, epoll_, buffer };
        }
        exec::Signal<std::uint32_t, std::error_code> auto write(std::span<const std::byte> buffer) & {
            return write_signal{ state_, epoll_, buffer };
        }

        // expected to always be called in the end
        result<meta::unit> unbind() &&;

    private:
        state::socket& state_;
        sys::epoll& epoll_;
        bool is_unbound_ = false;
    };

private:
    explicit socket(state::socket& a_socket);

public:
    [[nodiscard]] static result<std::unique_ptr<socket>> create(state::socket& a_socket);

    result<bound> bind(sys::epoll& an_epoll) &;

private:
    state::socket& state_;
    sys::epoll::callback callback_;
};

} // namespace sl::io::async
