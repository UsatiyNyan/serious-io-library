//
// Created by usatiynyan.
//

#pragma once

#include <sl/meta/monad/result.hpp>

namespace sl::io {

template <typename T>
using result = meta::result<T, std::error_code>;

template <typename T>
constexpr bool check_reschedule(const result<T>& r) {
    if (r.has_value()) {
        return false;
    }
    const auto ec = r.error();
    return ec == std::errc::resource_unavailable_try_again || ec == std::errc::operation_would_block;
}

} // namespace sl::io
