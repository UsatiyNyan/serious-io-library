//
// Created by usatiynyan.
//

#pragma once

#include <sl/meta/monad/result.hpp>

namespace sl::io {

template <typename T>
using result = meta::result<T, std::error_code>;

} // namespace sl::io
