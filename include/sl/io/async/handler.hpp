//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/sys/epoll.hpp"

#include <type_traits>

namespace sl::io {

enum class handler_result { resume, end };

template <typename F>
concept HandlerRequirement = std::is_nothrow_invocable_r_v<handler_result, F, epoll::event_flag>;

struct handler_base {
    virtual ~handler_base() = default;
    [[nodiscard]] virtual handler_result execute(epoll::event_flag event_flag) noexcept = 0;
};

template <HandlerRequirement F>
class handler : public handler_base {
public:
    template <typename FV>
    explicit handler(FV&& f) : f_{ std::forward<FV>(f) } {}

    [[nodiscard]] handler_result execute(epoll::event_flag event_flag) noexcept override { return f_(event_flag); }

private:
    F f_;
};

template <typename FV>
auto* allocate_handler(FV&& f) {
    return new (std::nothrow) handler<std::decay_t<FV>>{ std::forward<FV>(f) };
}

} // namespace sl::io
