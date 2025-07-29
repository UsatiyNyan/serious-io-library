//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/sys/socket.hpp"

#include <sl/meta/func/function.hpp>
#include <sl/meta/monad/maybe.hpp>
#include <sl/meta/traits/unique.hpp>

namespace sl::io::state {

struct server {
    using callback = meta::unique_function<void(result<std::pair<sys::socket, sys::address>>)>;

public:
    explicit server(sys::listening_server& a_server) : sys_{ a_server } {}

    const sys::listening_server& sys() const& { return sys_; }
    sys::listening_server& sys() & { return sys_; }

    void begin_accept(callback cb) &;
    void resume_accept() &;
    bool cancel_accept() &;

private:
    callback callback_{};
    sys::listening_server& sys_;
};

} // namespace sl::io::state
