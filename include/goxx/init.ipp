#pragma once
#include "goxx/init.hpp"
#include <ctime>

goxx_init([]() { std::srand((unsigned int)std::time(0)); });
namespace goxx {

Init::Init(std::function<void()> &&f) { f(); }

} // namespace goxx
