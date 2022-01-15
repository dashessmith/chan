#pragma once
#include "goxx/annoymous.hpp"
#include <functional>
namespace goxx {
#define goxx_init(f) inline static Init goxx_{f};
class Init {
  public:
    Init(std::function<void()> &&f);
};
} // namespace goxx

#include "goxx/init.ipp"