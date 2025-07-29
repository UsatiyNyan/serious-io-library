//
// Created by usatiynyan.
//

#include "sl/io/state/server.hpp"

#include <libassert/assert.hpp>

namespace sl::io::state {

void server::begin_accept(callback cb) & {
    DEBUG_ASSERT(!callback_);
    callback_ = std::move(cb);
    resume_accept();
}

void server::resume_accept() & {
    if (!DEBUG_ASSERT_VAL(callback_)) {
        return;
    }

    auto accept_result = sys_.accept();
    if (!accept_result.has_value()) {
        if (const auto ec = std::move(accept_result).error();
            ec == std::errc::resource_unavailable_try_again || ec == std::errc::operation_would_block) {
            // re-schedule
            return;
        }
    }

    std::exchange(callback_, callback{})(std::move(accept_result));
}

bool server::cancel_accept() & {
    const auto prev_callback = std::exchange(callback_, callback{});
    return !prev_callback.empty();
}

} // namespace sl::io::state
