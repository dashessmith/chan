#pragma once
#include <functional>
class Defer {
  public:
    inline Defer(std::function<void()> f) : f_{f} {}
    inline ~Defer() { f_(); }

  private:
    std::function<void()> f_;
};