#pragma once
#include <chrono>
#include <functional>

namespace goxx {
std::chrono::steady_clock::duration elapse(std::function<void()> f);

} // namespace goxx

#include "elapse.ipp"