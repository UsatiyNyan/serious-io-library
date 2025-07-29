//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/sys/socket.hpp"

#include <sl/meta/func/function.hpp>
#include <sl/meta/monad/maybe.hpp>
#include <sl/meta/traits/unique.hpp>

#include <variant>

namespace sl::io::state {

struct socket {
    using callback = meta::unique_function<void(result<std::uint32_t>)>;
    struct read_state {
        std::span<std::byte> buf;
        callback cb;
    };
    struct write_state {
        std::span<const std::byte> buf;
        callback cb;
    };
    using state = std::variant<read_state, write_state>;

public:
    explicit socket(sys::socket& a_socket) : sys_{ a_socket } {}

    const sys::socket& sys() const& { return sys_; }
    sys::socket& sys() & { return sys_; }

    void begin_read(std::span<std::byte> buf, callback cb) &;
    void begin_write(std::span<const std::byte> buf, callback cb) &;

    void resume_read() &;
    void resume_write() &;

    bool cancel_read() &;
    bool cancel_write() &;

    void handle_error() &;
    void handle_close() &;

private:
    void begin(state a_state) &;
    void resume() &;

private:
    meta::maybe<state> state_{};
    sys::socket& sys_;
};

} // namespace sl::io::state
