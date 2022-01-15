#pragma once
#include "goxx/defer.hpp"

namespace goxx {

Defer::Defer(std::function<void()> f) : f_{f} {}
Defer::~Defer() { f_(); }

} // namespace goxx