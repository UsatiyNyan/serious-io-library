//
// Created by usatiynyan.
//

#include "sl/eq/async_connection.hpp"

#include <sl/meta/lifetime/defer.hpp>

namespace sl::eq {

void async_connection::handle_error() {
    const auto socket_error = connection.socket.get_opt(SOL_SOCKET, SO_ERROR);
    handle_error_impl(
        socket_error.has_value() //
            ? std::make_error_code(std::errc{ socket_error.value() })
            : socket_error.error()
    );
}

void async_connection::handle_close() { handle_error_impl(std::make_error_code(std::errc::connection_reset)); }

void async_connection::handle_read() {
    if (!ASSUME_VAL(read_state_.has_value())) {
        return;
    }
    io::result<std::uint32_t> read_result = connection.socket.handle.read(read_state_->buffer);
    if (!read_result.has_value()) {
        const auto ec = read_result.error();
        if (ec == std::errc::resource_unavailable_try_again || ec == std::errc::operation_would_block) {
            // will retry
            return;
        }
    }
    meta::defer cleanup{ [this] { read_state_.reset(); } };
    read_state_->slot.set_value(std::move(read_result).value());
}

void async_connection::handle_write() {
    if (!ASSUME_VAL(write_state_.has_value())) {
        return;
    }
    io::result<std::uint32_t> write_result = connection.socket.handle.write(write_state_->buffer);
    if (!write_result.has_value()) {
        const auto ec = write_result.error();
        if (ec == std::errc::resource_unavailable_try_again || ec == std::errc::operation_would_block) {
            // will retry
            return;
        }
    }
    meta::defer cleanup{ [this] { write_state_.reset(); } };
    write_state_->slot.set_value(std::move(write_result).value());
}

void async_connection::handle_error_impl(std::error_code ec) {
    if (read_state_.has_value()) {
        meta::defer cleanup{ [this] { read_state_.reset(); } };
        read_state_->slot.set_error(std::error_code{ ec });
    }
    if (write_state_.has_value()) {
        meta::defer cleanup{ [this] { write_state_.reset(); } };
        write_state_->slot.set_error(std::error_code{ ec });
    }
}

void async_connection::begin_read(std::span<std::byte> buffer, slot_type& slot) {
    ASSERT(!read_state_.has_value());
    read_state_.emplace(buffer, slot);
    handle_read();
}

void async_connection::begin_write(std::span<const std::byte> buffer, slot_type& slot) {
    ASSERT(!write_state_.has_value());
    write_state_.emplace(buffer, slot);
    handle_write();
}

} // namespace sl::eq
