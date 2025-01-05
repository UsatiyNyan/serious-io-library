//
// Created by usatiynyan.
//

#include "sl/io/sys/epoll.hpp"

#include <gtest/gtest.h>

namespace sl::io {

TEST(sysEpoll, flags) {
    {
        epoll::event_flag reference_flags{ EPOLLOUT | EPOLLET };
        asm volatile("" : : "r,m"(reference_flags) : "memory");

        std::optional<bool> result;
        if (reference_flags & epoll::event::out) {
            result = true;
        } else if (reference_flags & epoll::event::in) {
            result = false;
        }
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result.value());
    }

    {
        epoll::event_flag reference_flags{ EPOLLIN | EPOLLET };
        asm volatile("" : : "r,m"(reference_flags) : "memory");

        std::optional<bool> result;
        if (reference_flags & epoll::event::in) {
            result = true;
        } else if (reference_flags & epoll::event::out) {
            result = false;
        }
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result.value());
    }
}

} // namespace sl::io
