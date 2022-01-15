#pragma once
#include "goxx/annoymous.hpp"
#include <functional>

namespace goxx {
class Init {
  public:
    Init(std::function<void()> &&f);
};
} // namespace goxx

#define goxx_init(f) inline static goxx::Init goxx_annoy{f};

#include "goxx/init.ipp"