#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
namespace goxx {
/*
    GO style wait group
 */
class WaitGroup {

  public:
    inline ~WaitGroup();

    inline void add(size_t n = 1);

    inline void done();

    inline void go(std::function<void()> &&f);

    inline void
    together(std::function<void(size_t thread_idx, size_t num_threads)> &&f,
             std::function<void()> &&final = nullptr,
             size_t nt = std::thread::hardware_concurrency());
    inline void wait();

  private:
    size_t count_ = 0;
    std::mutex mtx_;
    std::condition_variable cv_;
};

} // namespace goxx

#include "goxx/wait_group.ipp"