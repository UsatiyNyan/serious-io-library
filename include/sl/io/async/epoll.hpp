//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/async/connection.hpp"

#include "sl/io/sys/epoll.hpp"
#include "sl/io/sys/socket.hpp"

#include <function2/function2.hpp>
#include <sl/exec/algo/sched/inline.hpp>
#include <sl/exec/coro/async_gen.hpp>
#include <sl/exec/coro/await.hpp>
#include <sl/meta/lifetime/immovable.hpp>

#include <type_traits>

namespace sl::io {

struct async_epoll : meta::immovable {
    template <typename Signature>
    using handler_type = fu2::function_base<
        /*IsOwning=*/true,
        /*IsCopyable=*/false,
        /*Capacity=*/fu2::capacity_default,
        /*IsThrowing=*/false,
        /*HasStrongExceptGuarantee=*/true,
        Signature>;

    using slot_type = exec::slot<epoll::event_flag, std::error_code>;

    struct client_slot : slot_type {
        using deleter_type = handler_type<void(client_slot*, std::error_code)>;

        client_slot(epoll& epoll, socket::connection&& connection, deleter_type&& deleter)
            : async_connection{ std::move(connection) }, deleter_{ std::move(deleter) }, epoll_{ epoll } {
            epoll_
                .ctl(
                    epoll::op::add,
                    async_connection.connection.socket.handle,
                    ::epoll_event{
                        .events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLET,
                        .data{ .ptr = this },
                    }
                )
                .map_error([](std::error_code ec) { PANIC(ec); });
        }
        ~client_slot() override {
            epoll_.ctl(epoll::op::del, async_connection.connection.socket.handle, ::epoll_event{})
                .map_error([](std::error_code ec) { PANIC(ec); });
        }

        void set_value(epoll::event_flag&& events) & override {
            if (events & epoll::event::err) {
                async_connection.handle_error();
                cancel();
                return;
            }
            if ((events & epoll::event::rdhup) || (events & epoll::event::hup)) {
                async_connection.handle_close();
                cancel();
                return;
            }
            if (events & epoll::event::in) {
                async_connection.handle_read();
            } else if (events & epoll::event::out) {
                async_connection.handle_write();
            } else {
                PANIC(events, "connection: handle unknown events");
            }
        }

        void set_error(std::error_code&& ec) & override { deleter_(this, std::move(ec)); }
        void cancel() & override { deleter_(this, std::error_code{}); }

    public:
        async_connection async_connection;

    private:
        deleter_type deleter_;
        epoll& epoll_;
    };

    struct accept_connection : meta::immovable {
        accept_connection(async_epoll& self, slot_type& slot) : self_{ self }, slot_{ slot } {}

        ~accept_connection() {
            if (has_emitted_) {
                self_.epoll_.ctl(epoll::op::del, self_.server_.socket.handle, ::epoll_event{})
                    .map_error([](std::error_code ec) { PANIC(ec); });
            }
        }
        void emit() & {
            self_.epoll_
                .ctl(
                    epoll::op::add,
                    self_.server_.socket.handle,
                    ::epoll_event{ .events = EPOLLIN | EPOLLET, .data{ .ptr = &slot_ } }
                )
                .map_error([](std::error_code ec) { PANIC(ec); });
            has_emitted_ = true;
        }

    private:
        async_epoll& self_;
        slot_type& slot_;
        bool has_emitted_ = false;
    };

    struct accept_signal {
        using value_type = epoll::event_flag;
        using error_type = std::error_code;

        explicit accept_signal(async_epoll& self) : self_{ self } {}

        exec::Connection auto subscribe(slot_type& slot) && { return accept_connection{ self_, slot }; }
        exec::executor& get_executor() & { return exec::inline_executor(); }

    private:
        async_epoll& self_;
    };

public:
    async_epoll(epoll& epoll, socket::server& server)
        : epoll_{ epoll }, server_{ server },
          is_blocking_{ server_.socket.get_blocking().map_error([](std::error_code ec) { PANIC(ec); }).value() } {}

    exec::Signal auto accept() & { return accept_signal{ *this }; }

    exec::async_gen<async_connection::view, std::error_code> serve_coro(
        handler_type<bool()> check_running,
        handler_type<client_slot&(epoll&, socket::connection&&)> alloc_client = {}
    ) {
        using exec::operator co_await;
        if (!alloc_client) {
            alloc_client = [](epoll& epoll, socket::connection&& connection) -> client_slot& {
                return default_alloc_client(epoll, std::move(connection));
            };
        }

        while (check_running()) {
            auto maybe_events = co_await accept();
            if (!maybe_events.has_value()) {
                co_return std::move(maybe_events).error();
            }

            const auto events = std::move(maybe_events).value();
            DEBUG_ASSERT(events & epoll::event_flag{ epoll::event::in });

            while (check_running()) {
                auto accept_result = server_.accept();
                if (!accept_result.has_value()) {
                    const auto ec = std::move(accept_result).error();
                    if (ec == std::errc::resource_unavailable_try_again || ec == std::errc::operation_would_block) {
                        // re-schedule
                        break;
                    }
                    co_return ec;
                }

                auto connection = std::move(accept_result).value();
                connection.socket.set_blocking(is_blocking_).map_error([](std::error_code ec) { PANIC(ec); });
                auto& client_slot = alloc_client(epoll_, std::move(connection));
                co_yield async_connection::view{ client_slot.async_connection };
            }
        }
        co_return std::error_code{};
    }

    result<std::uint32_t>
        wait_and_fulfill(std::span<epoll_event> out_events, tl::optional<std::chrono::milliseconds> maybe_timeout) {
        result<std::uint32_t> wait_result = epoll_.wait(out_events, maybe_timeout);
        const std::uint32_t nevents = wait_result.value();
        for (const ::epoll_event& event : std::span{ out_events.data(), nevents }) {
            auto* slot_ptr = static_cast<slot_type*>(event.data.ptr);
            ASSERT(slot_ptr != nullptr);
            slot_ptr->set_value(epoll::event_flag{ event.events });
        }
        return wait_result;
    }

private:
    static client_slot& default_alloc_client(epoll& epoll, socket::connection&& connection) {
        auto* slot_ptr = new (std::nothrow) client_slot{
            epoll,
            std::move(connection),
            [](client_slot* self, std::error_code ec) noexcept {
                ASSERT(!ec);
                delete self;
            },
        };
        ASSERT(slot_ptr != nullptr);
        return *slot_ptr;
    }

private:
    epoll& epoll_;
    socket::server& server_;
    bool is_blocking_;
};

} // namespace sl::io
