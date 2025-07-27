//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/result.hpp"
#include "sl/io/sys/file.hpp"

#include <sl/meta/type/unit.hpp>

#include <variant>

#include <netinet/in.h>

namespace sl::io::sys {

using ipv4_address = ::sockaddr_in;
using ipv6_address = ::sockaddr_in6;
using address = std::variant<ipv4_address, ipv6_address>;

ipv4_address make_ipv4_address(std::uint16_t family, std::uint16_t port, std::uint32_t addr);
ipv6_address make_ipv6_address(
    std::uint16_t family,
    std::uint16_t port,
    std::uint32_t flowinfo,
    std::span<std::uint8_t, 16> addr,
    std::uint32_t scope_id
);
std::pair<const ::sockaddr*, ::socklen_t> inner_address(const address& an_address);
std::pair<::sockaddr*, ::socklen_t> inner_address(address& an_address);

struct bound_server;

struct socket {
    explicit socket(file a_file) : file_{ std::move(a_file) } {}

    [[nodiscard]] static result<socket> create(std::int32_t domain, std::int32_t type, std::int32_t protocol);

    [[nodiscard]] result<meta::unit> set_opt(std::int32_t level, std::int32_t name, std::int32_t opt) &;
    [[nodiscard]] result<std::int32_t> get_opt(std::int32_t level, std::int32_t name) const&;

    [[nodiscard]] result<std::uint32_t> set_blocking(bool is_blocking) &;
    [[nodiscard]] result<bool> get_blocking() const&;

    [[nodiscard]] result<bound_server> bind(address an_address) &&;

    const file& get_file() const& { return file_; }
    file& get_file() & { return file_; }

private:
    file file_;
};

struct listening_server;

struct bound_server {
private:
    friend struct socket;
    bound_server(socket a_socket, address an_address)
        : socket_{ std::move(a_socket) }, address_{ std::move(an_address) } {}

public:
    // backlog should be more than 0 and it will be truncated if exceeds 4096 by system
    [[nodiscard]] result<listening_server> listen(std::uint16_t backlog) &&;

private:
    socket socket_;
    address address_;
};

struct listening_server {
private:
    friend struct bound_server;
    explicit listening_server(socket a_socket, address an_address)
        : socket_{ std::move(a_socket) }, address_{ std::move(an_address) } {}

public:
    [[nodiscard]] result<std::pair<socket, address>> accept() &;

    const socket& get_socket() const& { return socket_; }
    socket& get_socket() & { return socket_; }

private:
    socket socket_;
    address address_;
};

} // namespace sl::io::sys
