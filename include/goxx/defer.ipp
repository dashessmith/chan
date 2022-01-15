#pragma once
#include "goxx/defer.hpp"

namespace goxx {

inline Defer::Defer(std::function<void()> f) : f_{f} {}
inline Defer::~Defer() { f_(); }

} // namespace goxx