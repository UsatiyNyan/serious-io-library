//
// Created by usatiynyan.
//

#include "sl/io/sys/socket.hpp"

#include <cstdint>
#include <libassert/assert.hpp>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sl/meta/match/overloaded.hpp>

namespace sl::io::sys {

ipv4_address make_ipv4_address(std::uint16_t family, std::uint16_t port, std::uint32_t addr) {
    return ipv4_address{
        .sin_family = family,
        .sin_port = ::htons(port),
        .sin_addr{ .s_addr = addr },
        .sin_zero{},
    };
}

ipv6_address make_ipv6_address(
    std::uint16_t family,
    std::uint16_t port,
    std::uint32_t flowinfo,
    std::span<std::uint8_t, 16> addr,
    std::uint32_t scope_id
) {
    ::in6_addr internal_addr;
    static_assert(addr.size_bytes() == sizeof(internal_addr.s6_addr));
    std::copy(addr.begin(), addr.end(), internal_addr.s6_addr);

    return ipv6_address{
        .sin6_family = family,
        .sin6_port = ::htons(port),
        .sin6_flowinfo = flowinfo,
        .sin6_addr = internal_addr,
        .sin6_scope_id = scope_id,
    };
}

std::pair<const ::sockaddr*, ::socklen_t> inner_address(const address& an_address) {
    return std::visit(
        [](const auto& inner) { return std::make_pair(std::bit_cast<const ::sockaddr*>(&inner), sizeof(inner)); },
        an_address
    );
}

std::pair<::sockaddr*, ::socklen_t> inner_address(address& an_address) {
    return std::visit(
        [](const auto& inner) { return std::make_pair(std::bit_cast<::sockaddr*>(&inner), sizeof(inner)); }, an_address
    );
}

result<socket> socket::create(std::int32_t domain, std::int32_t type, std::int32_t protocol) {
    const int sfd = ::socket(domain, type, protocol);
    if (sfd == -1) {
        return meta::errno_err();
    }
    return socket{ file{ sfd } };
}

result<meta::unit> socket::set_opt(std::int32_t level, std::int32_t name, std::int32_t opt) & {
    const int result = ::setsockopt(file_.internal(), level, name, &opt, sizeof(opt));
    if (result == -1) {
        return meta::errno_err();
    }
    return meta::unit{};
}

result<std::int32_t> socket::get_opt(std::int32_t level, std::int32_t name) const& {
    std::int32_t opt = 0;
    ::socklen_t optlen = sizeof(opt);
    const int result = ::getsockopt(file_.internal(), level, name, &opt, &optlen);
    ASSERT(optlen == sizeof(opt));
    if (result == -1) {
        return meta::errno_err();
    }
    return opt;
}

result<std::uint32_t> socket::set_blocking(bool is_blocking) & {
    return file_ //
        .fcntl(F_GETFL)
        .and_then([this, is_blocking](std::int32_t flags) {
            flags = is_blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
            return file_.fcntl(F_SETFL, flags);
        });
}

result<bool> socket::get_blocking() const& {
    return file_ //
        .fcntl(F_GETFL)
        .map([](std::int32_t flags) { return !(flags & O_NONBLOCK); });
}

result<bound_server> socket::bind(address an_address) && {
    const auto [ptr, sz] = inner_address(an_address);
    const int result = ::bind(file_.internal(), ptr, sz);
    if (result == -1) {
        return meta::errno_err();
    }
    return bound_server{ std::move(*this), std::move(an_address) };
}

result<listening_server> bound_server::listen(std::uint16_t backlog) && {
    ASSERT(backlog != 0);
    const int result = ::listen(socket_.get_file().internal(), static_cast<int>(backlog));
    if (result == -1) {
        return meta::errno_err();
    }
    return listening_server{ std::move(socket_), std::move(address_) };
}

result<std::pair<socket, address>> listening_server::accept() & {
    address client_address = std::visit(
        meta::overloaded{
            [](const ipv4_address&) -> address { return ipv4_address{}; },
            [](const ipv6_address&) -> address { return ipv6_address{}; },
        },
        address_
    );
    auto [ptr, sz] = inner_address(client_address);
    const int cfd = ::accept(socket_.get_file().internal(), ptr, &sz);
    if (cfd == -1) {
        return meta::errno_err();
    }
    DEBUG_ASSERT(sizeof(::sockaddr) == sz);
    return std::make_pair(socket{ file{ cfd } }, client_address);
}

} // namespace sl::io::sys
