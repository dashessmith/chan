#pragma once
#include "goxx/elapse.hpp"

namespace goxx {
auto elapse(std::function<void()> f) -> std::chrono::steady_clock::duration {
    auto t = std::chrono::steady_clock::now();
    f();
    return std::chrono::steady_clock::now() - t;
}
} // namespace goxx