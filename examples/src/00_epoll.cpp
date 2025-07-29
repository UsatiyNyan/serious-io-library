//
// Created by usatiynyan.
//

#include <sl/exec.hpp>
#include <sl/io.hpp>

#include <fmt/core.h>
#include <libassert/assert.hpp>

namespace sl {

exec::async<void> client_coro(
    std::pair<io::sys::socket, io::sys::address> accepted,
    io::sys::epoll& epoll,
    exec::executor& executor
) {
    using exec::operator co_await;
    using exec::operator|;

    auto& [socket, address] = accepted;
    io::state::socket socket_state{ socket };
    auto socket_async = *ASSERT_VAL(io::async::socket::create(socket_state));
    auto bound_socket_async = *ASSERT_VAL(socket_async->bind(epoll));
    meta::defer unbind_socket{ [&bound_socket_async] { ASSERT(std::move(bound_socket_async).unbind()); } };

    auto& client = bound_socket_async;

    fmt::println("started");
    while (true) {
        std::array<std::byte, 1024> buffer{};

        const auto read_result = co_await (client.read(buffer) | exec::continue_on(executor));
        if (!read_result.has_value() || *read_result == 0) {
            break;
        }
        std::string_view read_str{ reinterpret_cast<char*>(buffer.data()), *read_result };
        fmt::println("read {} bytes:\n{}", *read_result, read_str.data());

        const auto write_result =
            co_await (client.write(std::span{ buffer.data(), *read_result }) | exec::continue_on(executor));
        if (!write_result.has_value() || *write_result == 0) {
            break;
        }
        fmt::println("written {} bytes:\n{}", *write_result, read_str.data());
    }
    fmt::println("ended");
}

exec::async<void> serve_coro(io::async::server::bound server, io::sys::epoll& epoll, exec::executor& executor) {
    using exec::operator co_await;

    meta::defer unbind_server{ [&server] { ASSERT(std::move(server).unbind()); } };

    fmt::println("serve started");

    while (true) {
        auto accept_result = co_await server.accept();
        ASSERT(accept_result);
        fmt::println("accept");

        exec::coro_schedule(executor, client_coro(std::move(accept_result).value(), epoll, executor));
    }
}

void main(std::uint16_t port, std::uint16_t max_clients) {
    auto socket = *ASSERT_VAL(io::sys::socket::create(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0));
    ASSERT(socket.set_opt(SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 1));
    const auto address = io::sys::make_ipv4_address(AF_INET, port, INADDR_ANY);
    auto bound_server = *ASSERT_VAL(std::move(socket).bind(address));
    auto server = *ASSERT_VAL(std::move(bound_server).listen(max_clients));

    io::state::server server_state{ server };
    auto server_async = *ASSERT_VAL(io::async::server::create(server_state));

    auto epoll = *ASSERT_VAL(io::sys::epoll::create());
    auto bound_server_async = *ASSERT_VAL(server_async->bind(epoll));

    exec::manual_executor executor;
    exec::coro_schedule(executor, serve_coro(std::move(bound_server_async), epoll, executor));

    std::array<::epoll_event, 1024> events{};
    do {
        while (executor.execute_batch() > 0) {}
    } while (epoll.wait_and_fulfill(events, meta::null));
}

} // namespace sl

auto parse_args(int argc, const char* argv[]) {
    ASSERT(argc == 3, "port, max_clients");
    return std::make_tuple(
        static_cast<std::uint16_t>(std::strtoul(argv[1], nullptr, 10)),
        static_cast<std::uint16_t>(std::strtoul(argv[2], nullptr, 10))
    );
}

int main(int argc, const char* argv[]) {
    const auto [port, max_clients] = parse_args(argc, argv);
    sl::main(port, max_clients);
    return 0;
}
