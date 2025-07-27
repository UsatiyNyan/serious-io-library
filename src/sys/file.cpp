//
// Created by usatiynyan.
//

#include "sl/io/sys/file.hpp"

#include <libassert/assert.hpp>

#include <fcntl.h>
#include <unistd.h>

namespace sl::io::sys {

file::~file() noexcept {
    if (!fd_.has_value()) {
        return;
    }
    const int fd = fd_.value();
    const int result = ::close(fd);
    ASSERT(result == 0, meta::errno_code());
}

int file::internal() const {
    ASSERT(fd_.has_value());
    return *fd_;
}

int file::release() && {
    ASSERT(fd_.has_value());
    return *std::exchange(fd_, std::nullopt);
}

result<std::uint32_t> file::read(std::span<std::byte> buffer) & {
    ASSERT(fd_.has_value());
    const int nbytes = ::read(fd_.value(), buffer.data(), buffer.size());
    if (nbytes == -1) {
        return meta::errno_err();
    }
    return static_cast<std::uint32_t>(nbytes);
}

result<std::uint32_t> file::write(std::span<const std::byte> buffer) & {
    ASSERT(fd_.has_value());
    const int nbytes = ::write(fd_.value(), buffer.data(), buffer.size());
    if (nbytes == -1) {
        return meta::errno_err();
    }
    return static_cast<std::uint32_t>(nbytes);
}

result<std::int32_t> file::fcntl(std::int32_t cmd, std::int32_t arg) & {
    const int result = ::fcntl(fd_.value(), cmd, arg);
    if (result == -1) {
        return meta::errno_err();
    }
    return result;
}

result<std::int32_t> file::fcntl(std::int32_t cmd) const& {
    const int result = ::fcntl(fd_.value(), cmd, std::int32_t{});
    if (result == -1) {
        return meta::errno_err();
    }
    return result;
}

} // namespace sl::io::sys
