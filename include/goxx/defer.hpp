#pragma once
#include "goxx/annoymous.hpp"
#include <functional>

namespace goxx {

#define goxx_defer(f) Defer goxx_{f};
class Defer {
  public:
    inline Defer(std::function<void()> f);
    inline ~Defer();

  private:
    std::function<void()> f_;
};

} // namespace goxx

#include "goxx/defer.ipp"