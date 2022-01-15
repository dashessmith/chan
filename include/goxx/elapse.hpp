#pragma once
#include <chrono>
#include <functional>

namespace goxx {
auto elapse(std::function<void()> f) -> std::chrono::steady_clock::duration;
} // namespace goxx

#include "goxx/elapse.ipp"