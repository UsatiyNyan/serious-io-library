//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/async/connection.hpp"

#include "sl/io/sys/epoll.hpp"
#include "sl/io/sys/socket.hpp"

#include <sl/exec/coro/async.hpp>

#include <function2/function2.hpp>

namespace sl::io {

using make_client_coro = fu2::function_base<
    /*IsOwning=*/true,
    /*IsCopyable=*/false,
    /*Capacity=*/fu2::capacity_default,
    /*IsThrowing=*/false,
    /*HasStrongExceptGuarantee=*/true,
    exec::async<void>(async_connection::view)>;

void setup_server_handler(
    epoll& epoll,
    socket::server& server,
    exec::executor& executor,
    make_client_coro make_client_coro
);

void execute_events(std::span<const epoll_event> events);

} // namespace sl::io
