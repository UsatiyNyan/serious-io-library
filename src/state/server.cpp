//
// Created by usatiynyan.
//

#include "sl/io/state/server.hpp"

#include <sl/meta/assert.hpp>

namespace sl::io::state {

server::cancel_handle server::begin_accept(callbacks cbs) & {
    DEBUG_ASSERT(!cbs_.has_value());
    cbs_ = std::move(cbs);
    resume_accept();

    ++current_id_;
    return cancel_handle{ .self = *this, .id = current_id_ };
}

void server::resume_accept() & {
    if (!cbs_.has_value()) {
        return;
    }
    auto accept_result = sys_.accept();
    if (check_reschedule(accept_result)) {
        cbs_->epoll->defer();
        return;
    }
    auto cbs = std::exchange(cbs_, meta::null).value();
    cbs.epoll->eager();
    std::move(*cbs.slot).set_result(std::move(accept_result));
}

void server::cancel_accept(std::size_t id) & {
    if (id != current_id_) {
        return;
    }
    std::exchange(cbs_, meta::null) //
        .map([this](callbacks cbs) { std::move(*cbs.slot).set_result(meta::null); });
}

} // namespace sl::io::state
