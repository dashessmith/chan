#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

/*
    GO style wait group
 */
class WaitGroup {
    using TogetherFunc = std::function<void(size_t tidx, size_t nt)>;
    using GoFunc = std::function<void()>;

  public:
    WaitGroup() = default;
    WaitGroup(TogetherFunc &&f);
    WaitGroup(GoFunc &&f);
    ~WaitGroup();

    void add(size_t n = 1);

    void done();

    WaitGroup &go(GoFunc &&f);

    WaitGroup &together(TogetherFunc &&f,
                        size_t nt = std::thread::hardware_concurrency());
    void wait();

  private:
    size_t count_ = 0;
    std::mutex mtx_;
    std::condition_variable cv_;
};

WaitGroup::WaitGroup(TogetherFunc &&f) {
    this->together(std::forward<TogetherFunc>(f));
}

WaitGroup::WaitGroup(GoFunc &&f) { this->go(std::forward<GoFunc>(f)); }

WaitGroup::~WaitGroup() {
    // debug("???\n");
    wait();
}

void WaitGroup::add(size_t n) {
    std::unique_lock<std::mutex> _(mtx_);
    count_ += n;
}

void WaitGroup::done() {
    {
        std::unique_lock<std::mutex> _(mtx_);
        if (count_ > 0) {
            --count_;
        }
    }
    cv_.notify_all();
}
WaitGroup &WaitGroup::go(GoFunc &&f) {
    add();
    std::thread([this, f] {
        try {
            f();
            done();
        } catch (...) {
            done();
            throw;
        }
    }).detach();
    return *this;
}

WaitGroup &WaitGroup::together(WaitGroup::TogetherFunc &&f, size_t nt) {
    for (size_t tidx = 0; tidx < nt; tidx++) {
        this->go([tidx, nt, f] { f(tidx, nt); });
    }
    return *this;
}

void WaitGroup::wait() {
    std::unique_lock<std::mutex> ul(mtx_);
    cv_.wait(ul, [this] { return count_ <= 0; });
}