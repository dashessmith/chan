#pragma once
#include "annoymous.hpp"
#include <cstdlib>
#include <ctime>
namespace goxx {
#define goxx_init(f)                                                           \
    inline static int _ = [&]() {                                              \
        f();                                                                   \
        return 0;                                                              \
    }();

goxx_init([]() { std::srand((unsigned int)std::time(0)); });

// inline static int _ = [&]() {
//     [&]() { std::srand((unsigned int)std::time(0)); }();
//     return 0;
// }();
} // namespace goxx