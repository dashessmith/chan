#pragma once
#include "annoymous.hpp"
#include <functional>

namespace goxx {

#define goxx_defer(f) Defer goxx_{f};
class Defer {
  public:
    inline Defer(std::function<void()> f) : f_{f} {}
    inline ~Defer() { f_(); }

  private:
    std::function<void()> f_;
};

} // namespace goxx