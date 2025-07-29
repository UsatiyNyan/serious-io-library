//
// Created by usatiynyan.
//

#include "sl/io/state/socket.hpp"

#include <sl/meta/match/overloaded.hpp>

#include <libassert/assert.hpp>

namespace sl::io::state {

void socket::begin_read(std::span<std::byte> buf, callback cb) & {
    begin(read_state{
        .buf = buf,
        .cb = std::move(cb),
    });
}

void socket::begin_write(std::span<const std::byte> buf, callback cb) & {
    begin(write_state{
        .buf = buf,
        .cb = std::move(cb),
    });
}

void socket::resume_read() & {
    if (state_.has_value()) {
        resume();
    }
}

void socket::resume_write() & {
    if (state_.has_value()) {
        resume();
    }
}

bool socket::cancel_read() & {
    const bool result = state_.has_value() && std::holds_alternative<read_state>(state_.value());
    state_.reset();
    return result;
}

bool socket::cancel_write() & {
    const bool result = state_.has_value() && std::holds_alternative<write_state>(state_.value());
    state_.reset();
    return result;
}

void socket::handle_error() & {
    const auto socket_error_result = sys_.get_opt(SOL_SOCKET, SO_ERROR);
    const auto an_error = socket_error_result //
                              .map([](auto x) { return std::make_error_code(std::errc{ x }); })
                              .value_or(socket_error_result.error());
    std::exchange(state_, meta::null) //
        .map([an_error](auto state) { //
            std::visit([an_error](auto x) { x.cb(meta::err(an_error)); }, std::move(state));
        });
}

void socket::handle_close() & {
    const auto an_error = std::make_error_code(std::errc::connection_reset);
    std::exchange(state_, meta::null) //
        .map([an_error](auto state) { //
            std::visit([an_error](auto x) { x.cb(meta::err(an_error)); }, std::move(state));
        });
}

void socket::begin(state a_state) & {
    DEBUG_ASSERT(!state_.has_value());
    state_.emplace(std::move(a_state));
    resume();
}

void socket::resume() & {
    ASSERT_VAL(state_.has_value());

    auto& a_handle = sys_.get_file();
    const auto a_result = std::visit(
        meta::overloaded{
            [&a_handle](read_state& x) -> result<std::uint32_t> { return a_handle.read(x.buf); },
            [&a_handle](write_state& y) -> result<std::uint32_t> { return a_handle.write(y.buf); },
        },
        state_.value()
    );

    if (!a_result.has_value()) {
        if (const auto ec = a_result.error();
            ec == std::errc::resource_unavailable_try_again || ec == std::errc::operation_would_block) {
            // re-schedule
            return;
        }
    }

    std::visit( //
        [a_result](auto x) { x.cb(a_result); },
        std::exchange(state_, meta::null).value()
    );
}

} // namespace sl::io::state
