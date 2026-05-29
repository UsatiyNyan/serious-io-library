//
// Created by usatiynyan.
//

#include "sl/io/state/socket.hpp"

#include <sl/meta/match/overloaded.hpp>

#include <sl/meta/assert.hpp>
#include <variant>

namespace sl::io::state {

socket::read_cancel_handle socket::begin_read(std::span<std::byte> buf, callbacks cbs) & {
    DEBUG_ASSERT(!state_.has_value());
    state_.emplace(buf, std::move(cbs));
    resume_read();

    ++current_id_;
    return read_cancel_handle{
        .self = *this,
        .id = current_id_,
    };
}

socket::write_cancel_handle socket::begin_write(std::span<const std::byte> buf, callbacks cbs) & {
    DEBUG_ASSERT(!state_.has_value());
    state_.emplace(buf, std::move(cbs));
    resume_write();

    ++current_id_;
    return write_cancel_handle{
        .self = *this,
        .id = current_id_,
    };
}

void socket::resume_read() & {
    if (!state_.has_value()) {
        return;
    }
    read_buf* rb = std::get_if<read_buf>(&state_->buf);
    if (!rb) {
        return;
    }
    const auto result = sys_.get_file().read(*rb);
    if (check_reschedule(result)) {
        state_->cbs.epoll->defer();
        return;
    }
    set_result_impl(std::move(result));
}

void socket::resume_write() & {
    if (!state_.has_value()) {
        return;
    }
    write_buf* wb = std::get_if<write_buf>(&state_->buf);
    if (!wb) {
        return;
    }
    const auto result = sys_.get_file().write(*wb);
    if (check_reschedule(result)) {
        state_->cbs.epoll->defer();
        return;
    }
    set_result_impl(std::move(result));
}

void socket::handle_error() & {
    const auto socket_error_result = sys_.get_opt(SOL_SOCKET, SO_ERROR);
    const auto an_error = socket_error_result //
                              .map([](auto x) { return std::make_error_code(std::errc{ x }); })
                              .value_or(socket_error_result.error());
    if (state_.has_value()) {
        set_result_impl(meta::err(an_error));
    }
}

void socket::handle_close() & {
    const auto an_error = std::make_error_code(std::errc::connection_reset);
    if (state_.has_value()) {
        set_result_impl(meta::err(an_error));
    }
}

void socket::cancel_read(std::size_t id) & {
    if (!state_.has_value() || !std::holds_alternative<read_buf>(state_->buf) || id != current_id_) {
        return;
    }
    set_result_impl(meta::null);
}

void socket::cancel_write(std::size_t id) & {
    if (!state_.has_value() || !std::holds_alternative<write_buf>(state_->buf) || id != current_id_) {
        return;
    }
    set_result_impl(meta::null);
}

void socket::set_result_impl(meta::maybe<result<std::uint32_t>>&& r) {
    if (r.has_value()) {
        state_->cbs.epoll->eager();
    }
    // gentle dance, reset before set_result
    auto* slot = state_->cbs.slot;
    state_.reset();
    std::move(*slot).set_result(std::move(r));
}

} // namespace sl::io::state
