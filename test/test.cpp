#include "goxx/goxx.hpp"
#include <algorithm>
#include <any>
#include <chrono>
#include <cstdio>
#include <deque>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <iostream>
#include <map>
#include <mutex>
#include <variant>

using namespace std;

using namespace goxx;

void test_chan() {
    auto num_threads = thread::hardware_concurrency();
    for (auto verbose : {false})
        for (auto count : {1000000})
            for (auto nth : {num_threads})
                for (size_t csize :
                     {1024, 512, 128, 64, 32, 16, 8, 4, 2, 1,
                      0} /* {0, 1, 2, 4, 8, 16, 32, 64, 128, 512, 1024} */) {
                    Chan<int> c{csize};
                    auto d = elapse([&]() {
                        vector<int> collected;
                        mutex collected_mtx;
                        WaitGroup wg{};
                        if (nth <= 1) {
                            wg.go([&c, &count]() {
                                for (auto i = 0; i < count; i++) {
                                    c.push(std::move(decltype(i){i}));
                                }
                                c.close();
                            });
                            wg.go([&c, &verbose, &collected, &collected_mtx,
                                   &count]() {
                                for (auto n : c) {
                                    if (verbose) {
                                        collected_mtx.lock();
                                        collected.emplace_back(n);
                                        collected_mtx.unlock();
                                    }
                                }
                                fmt::print("consumer quit\n");
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
                                [&c, &verbose, &collected,
                                 &collected_mtx](size_t, size_t) {
                                    for (auto n : c) {
                                        if (verbose) {
                                            collected_mtx.lock();
                                            collected.emplace_back(n);
                                            collected_mtx.unlock();
                                        }
                                    }
                                },
                                nullptr, nth);
                        }
                        wg.wait();
                        fmt::print("wait group quit\n");
                        if (verbose) {
                            sort(collected.begin(), collected.end());
                            for (auto i = 0; i < count; i++) {

                                if (collected[i] != i) {
                                    vector<int> subvec{
                                        collected.begin() + i - 1,
                                        collected.begin() +
                                            std::min(size_t(i - 1 + 20),
                                                     collected.size())};
                                    fmt::print(
                                        "i={},collected[i] != i, {} != {}, "
                                        "csize={} {}\n",
                                        i, collected[i], i, csize, subvec);
                                    break;
                                    // throw runtime_error("assert failed");
                                }
                            }
                            if (collected.size() != count) {
                                fmt::print("size err {} != {}\n",
                                           collected.size(), count);
                                // throw runtime_error("not all collected");
                            }
                        }
                    });
                    if (!verbose) {
                        fmt::print("threads {}, count {}, chan size {}, elapse "
                                   "{} ms\n",
                                   nth, count, csize, d / 1ms);
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
            fmt::print("index {} quit\n", idx);
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
    fmt::print("start\n");
    auto d = elapse([&]() { sort(begin(vec), end(vec)); });
    fmt::print(" elapse {} ms\n", d / 1ms);
    if (!is_sorted(vec.begin(), vec.end())) {
        fmt::print(" test failed, not sorted");
        for (auto v : vec) {
            fmt::print("v {}", v);
        }
        fmt::print("\n");
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
class Shouter {
  public:
    Shouter() { fmt::print("im here\n"); }
    Shouter(const Shouter &other) { fmt::print("im here copied\n"); }
    Shouter(Shouter &&other) {
        fmt::print("im here by rvalue\n");
        other.has_data_ = false;
    }
    Shouter &operator=(const Shouter &other) {
        fmt::print("im assigned copied\n");
        return *this;
    }
    Shouter &operator=(Shouter &&other) {
        fmt::print("im assigned copied by rvalue\n");
        other.has_data_ = false;
        return *this;
    }
    ~Shouter() {
        if (has_data_) {
            fmt::print("i am gone with data!!\n");
        } else {
            fmt::print("i am gone\n");
        }
    }
    void foo() {}

  private:
    bool has_data_ = true;
};

template <>
struct fmt::formatter<Shouter> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(Shouter const &shouter, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "shouter ~~");
    }
};

void test_get() {

    auto x = goxx::get<vector<Shouter>>({"123"});
    x->emplace_back();
    x = goxx::get<vector<Shouter>>({"123", true});
    x = goxx::get<vector<Shouter>>({"123", true});
    x = goxx::get<vector<Shouter>>({"123", true});
    x->at(0).foo();
    x = goxx::get<vector<Shouter>>({"123"});
    x = goxx::get<vector<Shouter>>({"", true});
    x = goxx::get<vector<Shouter>>({"123"});
    x = goxx::get<vector<Shouter>>({"123456"});
}

void test_any() {
    auto ch = make_shared<Chan<any>>(1024);
    auto wg = make_shared<WaitGroup>();
    goxx_defer([&]() { wg->wait(); });
    wg->go([&]() {
        for (auto i = 0; i < 10; i++) {
            switch (i) {
            case 0:
                ch->push(0);
                break;
            case 1:
                ch->push(vector<int>{1, 2, 3});
                break;
            case 2:
                ch->push(map<string, string>{{"123", "456"}});
                break;
            case 3:
                ch->push("123fdsa");
                break;
            case 4:
                ch->push("jfidjfidss"s);
                break;
            default:
                ch->push(i);
                break;
            }
        }
        ch->close();
    });

    wg->go([&]() {
        for (auto x : *ch) {
            if (x.type() == typeid(int)) {
                cout << "x=" << any_cast<int>(x) << "\n";
            } else if (x.type() == typeid(string)) {
                cout << "x=" << any_cast<string>(x) << "\n";
            } else {
                cout << "x=" << x.type().name() << "\n";
            }
        }
    });
}

void test_variant() {
    auto ch = make_shared<
        Chan<variant<int, vector<int>, map<string, string>, string, Shouter>>>(
        1024);
    auto wg = make_shared<WaitGroup>();
    goxx_defer([&]() { wg->wait(); });
    wg->go([&]() {
        for (auto i = 0; i < 10; i++) {
            switch (i) {
            case 1:
                ch->push(vector<int>{1, 2, 3});
                break;
            case 2:
                ch->push(map<string, string>{{"123", "456"}});
                break;
            case 3:
                ch->push("123fdsa");
                break;
            case 4:
                ch->push("jfidjfidss"s);
                break;
            default:
                ch->push(i);
                break;
            }
        }
        ch->close();
    });

    wg->go([&]() {
        for (auto v : *ch) {
            visit(
                cases{
                    // [](const map<string, string> &arg) { fmt::print("map\n"); },
                    // [](const vector<int> &arg) { fmt::print("vec\n"); },
                    // [](const string &arg) { fmt::print("str {}\n", arg); },
                    [](const auto &arg) { fmt::print("what {}\n", arg); },
                },
                v);
        }
    });
}

void test_optional() {
    auto opt1 = optional<int>{1};
    auto opt2 = optional<int>{};
    fmt::print("opt1/opt2 has value ? {}/{}\n", opt1.has_value(),
               opt2.has_value());

    opt2.swap(opt1);
    // opt1.reset();
    fmt::print("opt1/opt2 has value ? {}/{}\n", opt1.has_value(),
               opt2.has_value());
}
int main() {
    // test_optional();
    // test_chan();
    //    test_mt_sort_origin();
    //    test_mt_sort();
    //    test_priority_queue();
    //    test_get();
    //   test_any();
    test_variant();
    return 0;
}