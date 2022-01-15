#pragma once
#include "goxx/annoymous.hpp"
#include <functional>

namespace goxx {

class Defer {
  public:
    Defer(std::function<void()> f);
    ~Defer();

  private:
    std::function<void()> f_;
};

} // namespace goxx

#define goxx_defer(f) Defer goxx_annoy{f};

#include "goxx/defer.ipp"