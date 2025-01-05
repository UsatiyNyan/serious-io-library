//
// Created by usatiynyan.
//

#include "sl/io/async/connection.hpp"

namespace sl::io {

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
    if (!read_state_.has_value()) {
        return;
    }
    result<std::uint32_t> read_result = connection.socket.handle.read(read_state_->buffer);
    if (!read_result.has_value()) {
        const auto ec = read_result.error();
        if (ec == std::errc::resource_unavailable_try_again || ec == std::errc::operation_would_block) {
            // will retry
            return;
        }
    }
    slot_type& slot = std::exchange(read_state_, tl::nullopt)->slot;
    slot.set_value(std::move(read_result).value());
}

void async_connection::handle_write() {
    if (!write_state_.has_value()) {
        return;
    }
    result<std::uint32_t> write_result = connection.socket.handle.write(write_state_->buffer);
    if (!write_result.has_value()) {
        const auto ec = write_result.error();
        if (ec == std::errc::resource_unavailable_try_again || ec == std::errc::operation_would_block) {
            // will retry
            return;
        }
    }
    slot_type& slot = std::exchange(write_state_, tl::nullopt)->slot;
    slot.set_value(std::move(write_result).value());
}

void async_connection::handle_error_impl(std::error_code ec) {
    if (read_state_.has_value()) {
        slot_type& slot = std::exchange(read_state_, tl::nullopt)->slot;
        slot.set_error(std::error_code{ ec });
    }
    if (write_state_.has_value()) {
        slot_type& slot = std::exchange(write_state_, tl::nullopt)->slot;
        slot.set_error(std::error_code{ ec });
    }
}

void async_connection::begin_read(std::span<std::byte> buffer, slot_type& slot) {
    ASSERT(!read_state_.has_value());
    read_state_.emplace(read_state{ buffer, slot });
    handle_read();
}

void async_connection::begin_write(std::span<const std::byte> buffer, slot_type& slot) {
    ASSERT(!write_state_.has_value());
    write_state_.emplace(write_state{ buffer, slot });
    handle_write();
}

} // namespace sl::io
