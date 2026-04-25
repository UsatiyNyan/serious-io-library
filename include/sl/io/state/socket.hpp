//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/sys/socket.hpp"

#include <sl/exec/model/slot.hpp>
#include <sl/meta/func/function.hpp>
#include <sl/meta/monad/maybe.hpp>
#include <sl/meta/traits/unique.hpp>

#include <variant>

namespace sl::io::state {

struct socket {
    using callback = exec::slot_callback<std::uint32_t, std::error_code>;
    using read_buf = std::span<std::byte>;
    using write_buf = std::span<const std::byte>;
    struct state {
        std::variant<read_buf, write_buf> buf;
        callback* cb;
    };

public:
    explicit socket(sys::socket& a_socket) : sys_{ a_socket } {}

    const sys::socket& sys() const& { return sys_; }
    sys::socket& sys() & { return sys_; }

    void begin_read(std::span<std::byte> buf, callback& cb) &;
    void begin_write(std::span<const std::byte> buf, callback& cb) &;

    void resume_read() &;
    void resume_write() &;

    void cancel_read() &;
    void cancel_write() &;

    void handle_error() &;
    void handle_close() &;

private:
    void set_result_impl(meta::maybe<result<std::uint32_t>>&& r);

private:
    meta::maybe<state> state_{};
    sys::socket& sys_;
};

} // namespace sl::io::state
