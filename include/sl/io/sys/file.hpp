//
// Created by usatiynyan.
//

#pragma once

#include "sl/io/result.hpp"

#include <sl/meta/lifetime/immovable.hpp>

#include <tl/optional.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

namespace sl::io {

struct file : meta::immovable {
    struct view;

public:
    explicit file(int fd) : fd_{ tl::in_place, fd } {}
    file(file&& other) : fd_{ std::move(other).release() } {}
    ~file() noexcept;

    [[nodiscard]] int internal() const;
    [[nodiscard]] int release() &&;

    [[nodiscard]] result<std::uint32_t> read(std::span<std::byte> buffer);
    [[nodiscard]] result<std::uint32_t> write(std::span<const std::byte> buffer);

    [[nodiscard]] result<std::int32_t> fcntl(std::int32_t op, std::int32_t arg);

private:
    tl::optional<int> fd_{};
};

struct file::view {
    explicit view(int fd) : fd_{ fd } {}
    explicit view(const file& a_file) : fd_{ a_file.internal() } {}

    [[nodiscard]] int internal() const { return fd_; }

private:
    int fd_;
};

} // namespace sl::io
