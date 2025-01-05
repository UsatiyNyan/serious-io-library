//
// Created by usatiynyan.
//

#include <sl/exec.hpp>
#include <sl/io.hpp>

#include <libassert/assert.hpp>

#include <iostream>

namespace sl {

void main(std::uint16_t port, std::uint16_t max_clients) {
    auto epoll = *ASSERT_VAL(io::epoll::create());

    io::socket::server server =
        io::socket::create(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)
            .and_then([](io::socket socket) {
                return socket.set_opt(SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 1)
                    .map([socket = std::move(socket)](meta::unit) mutable { return std::move(socket); });
            })
            .and_then([port](io::socket socket) { return std::move(socket).bind(AF_INET, port, INADDR_ANY); })
            .and_then([max_clients](io::socket::bound_server bound_server) {
                return std::move(bound_server).listen(max_clients);
            })
            .map_error([](std::error_code ec) { PANIC(ec); })
            .value();

    io::async_epoll async_epoll{ epoll, server };

    constexpr auto client_handle_coro = [](io::async_connection::view conn) -> exec::async<void> {
        using exec::operator co_await;

        std::cout << "start client\n";
        while (true) {
            std::array<std::byte, 1024> buffer{};

            const auto read_result = co_await conn.read(buffer);
            if (!read_result.has_value() || *read_result == 0) {
                break;
            }
            std::string_view string_view{ reinterpret_cast<char*>(buffer.data()), *read_result };
            std::cout << "got:{" << string_view << "}\n";

            const auto write_result = co_await conn.write(std::span{ buffer.data(), *read_result });
            if (!write_result.has_value() || *write_result == 0) {
                break;
            }
            std::cout << "read: " << *read_result << " written: " << *write_result << "\n";
        }
        std::cout << "end client\n";
    };

    exec::manual_executor executor;
    exec::coro_schedule(executor, [&async_epoll, &executor, client_handle_coro] -> exec::async<void> {
        const auto ec = co_await async_epoll.serve_coro(
            [] { return true; },
            [&executor, client_handle_coro](io::async_connection::view conn) {
                exec::coro_schedule(executor, client_handle_coro(std::move(conn)));
            }
        );
        ASSERT(!ec);
    }());

    executor.execute_batch();

    std::array<::epoll_event, 1024> events{};
    while (async_epoll.wait_and_fulfill(events, tl::nullopt)) {
        while (executor.execute_batch() > 0) {
            std::cout << "spin\n";
        }
    }
}

} // namespace sl

auto parse_args(int argc, const char* argv[]) {
    ASSERT(argc == 3);
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
