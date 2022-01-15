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
    ~WaitGroup();

    void add(size_t n = 1);

    void done();

    void go(std::function<void()> &&f);

    void
    together(std::function<void(size_t thread_idx, size_t num_threads)> &&f,
             std::function<void()> &&final = nullptr,
             size_t nt = std::thread::hardware_concurrency());
    void wait();

  private:
    size_t count_ = 0;
    std::mutex mtx_;
    std::condition_variable cv_;
};

} // namespace goxx

#include "goxx/wait_group.ipp"