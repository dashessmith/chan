#include "goxx.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <deque>
#include <iostream>
#include <mutex>

using namespace goxx;

void test_chan() {
    Chan<void *> ch;
    auto num_threads = std::thread::hardware_concurrency();
    for (auto v : std::vector<bool>{true, false})
        for (auto count : std::vector<int>{1000000})
            for (auto nth : std::vector<size_t>{num_threads})
                for (auto csize : std::vector<size_t>{0, 1, 2, 3, 1024}) {
                    Chan<int> c{csize};
                    auto d = elapse([&]() {
                        std::vector<int> collected;
                        std::mutex collected_mtx;
                        WaitGroup wg{};
                        if (nth <= 1) {
                            wg.go([&c, &count]() {
                                for (auto i = 0; i < count; i++) {
                                    c.push(i);
                                }
                                c.close();
                            });
                            wg.go(
                                [&c, &v, &collected, &collected_mtx, &count]() {
                                    for (auto n : c) {
                                        if (v) {
                                            collected_mtx.lock();
                                            collected.emplace_back(n);
                                            collected_mtx.unlock();
                                        }
                                    }
                                    std::cout << " consumer quit\n";
                                });

                        } else {
                            wg.together(
                                [&c, &count](size_t tidx, size_t nthds) {
                                    for (auto i = tidx; i < count; i += nthds) {
                                        c.push((int)i);
                                    }
                                },
                                [&c]() { c.close(); }, nth);
                            wg.together(
                                [&c, &v, &collected, &collected_mtx](size_t,
                                                                     size_t) {
                                    for (auto n : c) {
                                        if (v) {
                                            collected_mtx.lock();
                                            collected.emplace_back(n);
                                            collected_mtx.unlock();
                                        }
                                    }
                                },
                                nullptr, nth);
                        }
                        wg.wait();
                        std::cout << "wait group quit\n";
                        if (v) {
                            sort(collected.begin(), collected.end());
                            for (auto i = 0; i < count; i++) {
                                if (collected[i] != i) {
                                    std::cout << "collected[i] != i, "
                                              << collected[i] << " != " << i
                                              << "\n";
                                    throw std::runtime_error("assert failed");
                                }
                            }
                        }
                    });
                    if (!v) {
                        std::cout << "threads " << nth << ", count " << count
                                  << ", chan size  " << csize << ", elapse "
                                  << d / std::chrono::milliseconds(1)
                                  << " ms\n";
                    }
                }
}

void test1() {
    WaitGroup wg{};
    int x = 0;
    std::mutex mtx;
    // std::condition_variable cv;
    wg.together([&](size_t idx, size_t) {
        for (;;) {
            std::unique_lock<std::mutex> ul(mtx);
            if (x != idx) {
                continue;
            }
            x++;
            std::cout << " index " << idx << " quit\n";
            break;
        }
    });
}
void test_mt_sort_origin() {
    size_t N = 100000000;
    std::vector<size_t> vec(N);
    for (size_t i = 0; i < N; i++) {
        vec[i] = (size_t)std::rand() / N;
    }
    // vec = {1118, 314, 1634, 10, 998, 2102, 2345, 2332, 3186, 3225};
    std::cout << "start\n";
    auto d = elapse([&]() { std::sort(std::begin(vec), std::end(vec)); });
    std::cout << " elapse " << (d / std::chrono::milliseconds(1)) << "ms\n";
    if (!std::is_sorted(vec.begin(), vec.end())) {
        std::cout << " test failed, not sorted\n";
        for (auto v : vec) {
            std::cout << v << " ";
        }
        std::cout << "\n";
    } else {
        // std::cout << " elapse " << (d / std::chrono::milliseconds(1)) <<
        // "ms\n";
    }
}

void test_mt_sort() {
    size_t N = 10000;
    std::vector<size_t> vec(N);
    for (size_t i = 0; i < N; i++) {
        vec[i] = (size_t)std::rand() / N;
    }
    // vec = {1118, 314, 1634, 10, 998, 2102, 2345, 2332, 3186, 3225};
    std::cout << "start\n";
    auto d = elapse([&]() { mt_sort(std::begin(vec), std::end(vec)); });
    std::cout << " elapse " << (d / std::chrono::milliseconds(1)) << "ms\n";
    if (!std::is_sorted(vec.begin(), vec.end())) {
        std::cout << " test failed, not sorted\n";
        for (auto v : vec) {
            std::cout << v << " ";
        }
        std::cout << "\n";
    } else {
        // std::cout << " elapse " << (d / std::chrono::milliseconds(1)) <<
        // "ms\n";
    }
}

int main() {
    test_mt_sort_origin();
    test_mt_sort();
    return 0;
}