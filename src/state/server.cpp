//
// Created by usatiynyan.
//

#include "sl/io/state/server.hpp"

#include <sl/meta/assert.hpp>

namespace sl::io::state {

void server::begin_accept(callback& cb) & {
    DEBUG_ASSERT(!cb_.has_value());
    cb_ = cb;
    resume_accept();
}

void server::resume_accept() & {
    if (!cb_.has_value()) {
        return;
    }
    auto accept_result = sys_.accept();
    if (check_reschedule(accept_result)) {
        return;
    }
    auto& cb = std::exchange(cb_, meta::null).value();
    std::move(cb).set_result(std::move(accept_result));
}

void server::cancel_accept() & {
    std::exchange(cb_, meta::null) //
        .map([this](callback& cb) { std::move(cb).set_result(meta::null); });
}

} // namespace sl::io::state
