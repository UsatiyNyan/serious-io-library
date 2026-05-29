//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/sys/epoll.hpp"
#include "sl/io/sys/socket.hpp"

#include <sl/exec/model/slot.hpp>
#include <sl/meta/monad/maybe.hpp>
#include <sl/meta/traits/unique.hpp>

#include <variant>

namespace sl::io::state {

struct socket {
    using slot_callback = exec::slot_callback<std::uint32_t, std::error_code>;
    struct callbacks {
        sys::epoll::callback* epoll;
        slot_callback* slot;
    };

    using read_buf = std::span<std::byte>;
    using write_buf = std::span<const std::byte>;
    struct state {
        std::variant<read_buf, write_buf> buf;
        callbacks cbs;
    };

    struct read_cancel_handle {
        // CancelHandle
        constexpr void try_cancel() && noexcept { self.cancel_read(id); }

    public:
        socket& self;
        std::size_t id = 0;
    };

    struct write_cancel_handle {
        // CancelHandle
        constexpr void try_cancel() && noexcept { self.cancel_write(id); }

    public:
        socket& self;
        std::size_t id = 0;
    };

public:
    explicit socket(sys::socket& a_socket) : sys_{ a_socket } {}

    const sys::socket& sys() const& { return sys_; }
    sys::socket& sys() & { return sys_; }

    read_cancel_handle begin_read(std::span<std::byte> buf, callbacks cbs) &;
    write_cancel_handle begin_write(std::span<const std::byte> buf, callbacks cbs) &;

    void resume_read() &;
    void resume_write() &;

    void handle_error() &;
    void handle_close() &;

private:
    void cancel_read(std::size_t id) &;
    void cancel_write(std::size_t id) &;

    void set_result_impl(meta::maybe<result<std::uint32_t>>&& r);

private:
    meta::maybe<state> state_{};
    sys::socket& sys_;
    std::size_t current_id_ = 0;
};

} // namespace sl::io::state
