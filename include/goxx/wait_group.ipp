#pragma once

#include "goxx/wait_group.hpp"

namespace goxx {

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
void WaitGroup::go(std::function<void()> &&f) {
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
}

void WaitGroup::together(
    std::function<void(size_t thread_idx, size_t num_threads)> &&f,
    std::function<void()> &&final, size_t nt) {
    if (!final) {
        for (size_t tidx = 0; tidx < nt; tidx++) {
            this->go([tidx, nt, f] { f(tidx, nt); });
        }
        return;
    }
    auto inner = std::shared_ptr<WaitGroup>{};
    inner->together([f](size_t tidx, size_t nthds) { f(tidx, nthds); }, nullptr,
                    nt);
    this->go([inner, final]() {
        inner->wait();
        final();
    });
}

void WaitGroup::wait() {
    std::unique_lock<std::mutex> ul(mtx_);
    cv_.wait(ul, [this] { return count_ <= 0; });
}
} // namespace goxx