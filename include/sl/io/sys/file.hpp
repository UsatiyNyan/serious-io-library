//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/result.hpp"

#include <sl/meta/traits/unique.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace sl::io::sys {

struct file : meta::immovable {
    explicit file(int fd) : fd_{ std::in_place, fd } {}
    file(file&& other) : fd_{ std::move(other).release() } {}
    ~file() noexcept;

    [[nodiscard]] int internal() const;
    [[nodiscard]] int release() &&;

    [[nodiscard]] result<std::uint32_t> read(std::span<std::byte> buffer) &;
    [[nodiscard]] result<std::uint32_t> write(std::span<const std::byte> buffer) &;

    [[nodiscard]] result<std::int32_t> fcntl(std::int32_t op, std::int32_t arg) &;
    [[nodiscard]] result<std::int32_t> fcntl(std::int32_t op) const&;

private:
    std::optional<int> fd_;
};

} // namespace sl::io::sys
