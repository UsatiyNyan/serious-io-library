//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/sys/socket.hpp"

#include <sl/exec/algo/sched/inline.hpp>
#include <sl/exec/model.hpp>

#include <sl/meta/monad/maybe.hpp>
#include <sl/meta/monad/result.hpp>

namespace sl::io {

struct async_connection {
    struct read_connection;
    struct write_connection;
    struct read_signal;
    struct write_signal;
    struct view;

private:
    using slot_type = exec::slot<std::uint32_t, std::error_code>;
    struct read_state {
        std::span<std::byte> buffer;
        slot_type& slot;
    };
    struct write_state {
        std::span<const std::byte> buffer;
        slot_type& slot;
    };

public:
    explicit async_connection(socket::connection connection) : connection{ std::move(connection) } {}

    void handle_error();
    void handle_close();
    void handle_read();
    void handle_write();

private:
    void handle_error_impl(std::error_code ec);
    void begin_read(std::span<std::byte> buffer, slot_type& slot);
    void begin_write(std::span<const std::byte> buffer, slot_type& slot);

public:
    socket::connection connection;

private:
    meta::maybe<read_state> read_state_{};
    meta::maybe<write_state> write_state_{};
};

struct [[nodiscard]] async_connection::read_connection {
    std::span<std::byte> buffer;
    slot_type& slot;
    async_connection& self;

    void emit() && noexcept { self.begin_read(buffer, slot); }
};

struct [[nodiscard]] async_connection::write_connection {
    std::span<const std::byte> buffer;
    slot_type& slot;
    async_connection& self;

    void emit() && noexcept { self.begin_write(buffer, slot); }
};

struct [[nodiscard]] async_connection::read_signal {
    using value_type = std::uint32_t;
    using error_type = std::error_code;

    std::span<std::byte> buffer;
    async_connection& self;

    exec::Connection auto subscribe(slot_type& slot) && { return read_connection{ buffer, slot, self }; }
    exec::executor& get_executor() { return exec::inline_executor(); }
};

struct [[nodiscard]] async_connection::write_signal {
    using value_type = std::uint32_t;
    using error_type = std::error_code;

    std::span<const std::byte> buffer;
    async_connection& self;

    exec::Connection auto subscribe(slot_type& slot) && { return write_connection{ buffer, slot, self }; }
    exec::executor& get_executor() { return exec::inline_executor(); }
};

struct [[nodiscard]] async_connection::view {
    async_connection& self;

    exec::Signal auto read(std::span<std::byte> buffer) { return read_signal{ buffer, self }; }
    exec::Signal auto write(std::span<const std::byte> buffer) { return write_signal{ buffer, self }; }
};

} // namespace sl::io
