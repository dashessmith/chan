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

} // namespace goxx

#include "init.ipp"