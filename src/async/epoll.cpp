//
// Created by usatiynyan.
//

#include "sl/io/async/epoll.hpp"
#include "sl/io/async/handler.hpp"
#include "sl/io/sys/socket.hpp"

#include <sl/meta/lifetime/defer.hpp>

#include <libassert/assert.hpp>

namespace sl::io {
namespace detail {

class connection_handler : public handler_base {
public:
    connection_handler(epoll& epoll, async_connection an_async_connection)
        : epoll_{ epoll }, async_connection_{ std::move(an_async_connection) } {}

    [[nodiscard]] handler_result execute(epoll::event_flag events) noexcept override {
        if (events & EPOLLERR) {
            async_connection_.handle_error();
            return handler_result::end;
        }
        if (events & (EPOLLRDHUP | EPOLLHUP)) {
            async_connection_.handle_close();
            return handler_result::end;
        }
        if (events & EPOLLIN) {
            async_connection_.handle_read();
        } else if (events & EPOLLOUT) {
            async_connection_.handle_write();
        } else {
            PANIC(events, "connection: handle unknown events");
        }
        return handler_result::resume;
    }

    ~connection_handler() override {
        epoll_ //
            .ctl(epoll::op::del, async_connection_.connection.socket.handle, ::epoll_event{})
            .map_error([](std::error_code ec) { PANIC(ec); });
    };

    [[nodiscard]] auto async_connection_view() { return async_connection::view{ async_connection_ }; }

private:
    epoll& epoll_;
    async_connection async_connection_;
};

::epoll_event make_server_handler(
    epoll& epoll,
    socket::server& server,
    exec::executor& executor,
    make_client_coro make_client_coro
) {
    const bool is_blocking = server
                                 .socket //
                                 .get_blocking()
                                 .map_error([](std::error_code ec) { PANIC(ec); })
                                 .value();

    auto* server_handler = allocate_handler( //
        [&epoll,
         &server,
         &executor,
         make_client_coro = std::move(make_client_coro),
         is_blocking,
         close_server = meta::defer{ [&epoll, handle = file::view{ server.socket.handle }] {
             epoll //
                 .ctl(epoll::op::del, handle, ::epoll_event{})
                 .map_error([](std::error_code ec) { PANIC(ec); });
         } }] //
        (epoll::event_flag events) mutable noexcept {
            while (true) {
                auto accept_result = server.accept();
                if (!accept_result.has_value()) {
                    const auto ec = std::move(accept_result).error();
                    if (ec == std::errc::resource_unavailable_try_again || ec == std::errc::operation_would_block) {
                        return handler_result::resume;
                    } else {
                        PANIC(ec, "TODO: determine when server should be closed");
                        return handler_result::end;
                    }
                }

                auto connection = std::move(accept_result).value();
                connection.socket.set_blocking(is_blocking).map_error([](std::error_code ec) { PANIC(ec); });

                const file::view handle_view{ connection.socket.handle };

                auto* a_connection_handler =
                    new (std::nothrow) connection_handler{ epoll, async_connection{ std::move(connection) } };
                ASSERT(a_connection_handler != nullptr);
                epoll
                    .ctl(
                        epoll::op::add,
                        handle_view,
                        ::epoll_event{
                            .events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLET,
                            .data{ .ptr = a_connection_handler },
                        }
                    )
                    .map_error([](std::error_code ec) { PANIC(ec); });

                exec::coro_schedule(executor, make_client_coro(a_connection_handler->async_connection_view()));
            }
        }
    );
    return ::epoll_event{
        .events = EPOLLIN | EPOLLET,
        .data{ .ptr = server_handler },
    };
}

} // namespace detail

void setup_server_handler(
    epoll& epoll,
    socket::server& server,
    exec::executor& executor,
    make_client_coro make_client_coro
) {
    epoll
        .ctl(
            epoll::op::add,
            server.socket.handle,
            detail::make_server_handler(epoll, server, executor, std::move(make_client_coro))
        )
        .map_error([](std::error_code ec) { PANIC(ec); });
}

void execute_events(std::span<const epoll_event> events) {
    for (const ::epoll_event& event : events) {
        auto* handler_ptr = static_cast<handler_base*>(event.data.ptr);
        ASSERT(handler_ptr != nullptr);
        const auto result = handler_ptr->execute(epoll::event_flag{ event.events });
        if (result == handler_result::end) {
            // assuming that client is removed from epoll here
            // and server communicates to stop executing events
            delete handler_ptr;
        }
    }
}

} // namespace sl::io
