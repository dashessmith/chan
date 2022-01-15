#include "goxx/goxx.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <deque>
#include <iostream>
#include <mutex>

using namespace std;

using namespace goxx;

void test_chan() {
    Chan<void *> ch;
    auto num_threads = thread::hardware_concurrency();
    for (auto v : {true, false})
        for (auto count : {1000000})
            for (auto nth : {num_threads})
                for (size_t csize : {0, 1, 2, 3, 1024}) {
                    Chan<int> c{csize};
                    auto d = elapse([&]() {
                        vector<int> collected;
                        mutex collected_mtx;
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
                                    cout << " consumer quit\n";
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
                        cout << "wait group quit\n";
                        if (v) {
                            sort(collected.begin(), collected.end());
                            for (auto i = 0; i < count; i++) {
                                if (collected[i] != i) {
                                    cout << "collected[i] != i, "
                                         << collected[i] << " != " << i << "\n";
                                    throw runtime_error("assert failed");
                                }
                            }
                        }
                    });
                    if (!v) {
                        cout << "threads " << nth << ", count " << count
                             << ", chan size  " << csize << ", elapse "
                             << d / chrono::milliseconds(1) << " ms\n";
                    }
                }
}

void test_wg() {
    WaitGroup wg{};
    int x = 0;
    mutex mtx;
    // condition_variable cv;
    wg.together([&](size_t idx, size_t) {
        for (;;) {
            unique_lock<mutex> ul(mtx);
            if (x != idx) {
                continue;
            }
            x++;
            cout << " index " << idx << " quit\n";
            break;
        }
    });
}
void test_mt_sort_origin() {
    size_t N = 100000000;
    vector<size_t> vec(N);
    for (size_t i = 0; i < N; i++) {
        vec[i] = (size_t)rand() / N;
    }
    // vec = {1118, 314, 1634, 10, 998, 2102, 2345, 2332, 3186, 3225};
    cout << "start\n";
    auto d = elapse([&]() { sort(begin(vec), end(vec)); });
    cout << " elapse " << (d / chrono::milliseconds(1)) << "ms\n";
    if (!is_sorted(vec.begin(), vec.end())) {
        cout << " test failed, not sorted\n";
        for (auto v : vec) {
            cout << v << " ";
        }
        cout << "\n";
    } else {
        // cout << " elapse " << (d / chrono::milliseconds(1)) <<
        // "ms\n";
    }
}

void test_mt_sort() {
    size_t N = 10000;
    vector<size_t> vec(N);
    for (size_t i = 0; i < N; i++) {
        vec[i] = (size_t)rand() / N;
    }
    // vec = {1118, 314, 1634, 10, 998, 2102, 2345, 2332, 3186, 3225};
    cout << "start\n";
    auto d = elapse([&]() { mt_sort(begin(vec), end(vec)); });
    cout << " elapse " << (d / chrono::milliseconds(1)) << "ms\n";
    if (!is_sorted(vec.begin(), vec.end())) {
        cout << " test failed, not sorted\n";
        for (auto v : vec) {
            cout << v << " ";
        }
        cout << "\n";
    } else {
        // cout << " elapse " << (d / chrono::milliseconds(1)) <<
        // "ms\n";
    }
}

void test_priority_queue() {
    priority_queue<int> q;
    for (int i = 0; i < 10; i++)
        q.push(10 - i);
    for (; !q.empty();) {
        auto n = q.top();
        q.pop();
        cout << "n = " << n << "\n";
    }
}

void test_get() {
    class Shouter {
      public:
        Shouter() { cout << "im here\n"; }
        ~Shouter() { cout << "i am gone!!\n"; }
    };
    auto x = goxx::get<Shouter>({.tag = "123"});
    x = goxx::get<Shouter>({.tag = "123", .permanent = true});
    x = goxx::get<Shouter>({.tag = "123", .permanent = true});
    x = goxx::get<Shouter>({.tag = "123", .permanent = true});
    x = goxx::get<Shouter>({.tag = "123"});
    x = goxx::get<Shouter>({.permanent = true});
    x = goxx::get<Shouter>({.tag = "123"});
    x = goxx::get<Shouter>({.tag = "123456"});
}

int main() {

    // test_mt_sort_origin();
    // test_mt_sort();
    // test_priority_queue();
    test_get();
    return 0;
}