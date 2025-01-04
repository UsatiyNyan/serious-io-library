//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/result.hpp"
#include "sl/io/sys/file.hpp"

#include <sl/meta/func/unit.hpp>

#include <netinet/in.h>

namespace sl::io {

struct socket {
    struct bound_server;
    struct listening_server;
    using server = listening_server;
    struct connection;

public:
    [[nodiscard]] static result<socket> create(std::int32_t domain, std::int32_t type, std::int32_t protocol);

    [[nodiscard]] result<meta::unit> set_opt(std::int32_t level, std::int32_t name, std::int32_t opt);
    [[nodiscard]] result<std::int32_t> get_opt(std::int32_t level, std::int32_t name);

    [[nodiscard]] result<meta::unit> set_blocking(bool is_blocking);
    [[nodiscard]] result<bool> get_blocking();

    [[nodiscard]] result<bound_server> bind(std::uint16_t family, std::uint16_t port, std::uint32_t in_addr) &&;

public:
    file handle;
};

struct socket::bound_server {
    // backlog should be more than 0 and it will be truncated if exceeds 4096 by system
    [[nodiscard]] result<listening_server> listen(std::uint16_t backlog) &&;

public:
    socket socket;
};

struct socket::listening_server {
    [[nodiscard]] result<socket::connection> accept();

public:
    socket socket;
};

struct socket::connection {
    socket socket;
    ::sockaddr_in address; // TODO: ipv6
};

} // namespace sl::io
