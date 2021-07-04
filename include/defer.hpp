#pragma once
#include <functional>
class Defer {
  public:
    Defer(std::function<void()> f) : f_{f} {}
    ~Defer() { f_(); }

  private:
    std::function<void()> f_;
};