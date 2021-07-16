#pragma once
#include "init.hpp"
namespace goxx {

goxx_init([]() { std::srand((unsigned int)std::time(0)); });

} // namespace goxx