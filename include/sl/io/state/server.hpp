//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/sys/socket.hpp"

#include <sl/exec/model/slot.hpp>
#include <sl/meta/func/function.hpp>
#include <sl/meta/monad/maybe.hpp>
#include <sl/meta/traits/unique.hpp>

namespace sl::io::state {

struct server {
    using callback = exec::slot_callback<std::pair<sys::socket, sys::address>, std::error_code>;

    struct cancel_handle {
        // CancelHandle
        constexpr void try_cancel() && noexcept { self.cancel_accept(id); }

    public:
        server& self;
        std::size_t id = 0;
    };

public:
    explicit server(sys::listening_server& a_server) : sys_{ a_server } {}

    const sys::listening_server& sys() const& { return sys_; }
    sys::listening_server& sys() & { return sys_; }

    cancel_handle begin_accept(callback& cb) &;
    void resume_accept() &;

private:
    void cancel_accept(std::size_t id) &;

private:
    meta::maybe<callback&> cb_{};
    sys::listening_server& sys_;
    std::size_t current_id_ = 0;
};

} // namespace sl::io::state
