#pragma once
#include "goxx/chan.hpp"
#include "goxx/wait_group.hpp"
#include <cstdlib>
#include <functional>
#include <iterator>

namespace goxx {
template <class Iterator>
void mt_sort(Iterator first, Iterator last) {
    mt_sort(first, last, std::less<>{});
}

template <class Iterator, class Compare>
void mt_sort(Iterator first, Iterator last, Compare comp) {
    auto N = std::distance(first, last);
    if (N <= 1)
        return;

    auto first_pivot = true;

    WaitGroup wg{};
    // Chan<std::pair<Iterator, Iterator>>
    // ch{std::thread::hardware_concurrency()};
    auto thread_limit = (1 << 10);

    std::function<void(Iterator, Iterator)> impl = [&](auto left, auto right) {
        auto d = std::distance(left, right);
        if (d <= 0) {
            return;
        }
        if (d == 1) {
            if (comp(*right, *left)) {
                std::swap(*left, *right);
            }
            return;
        }
        auto pivot = left;
        if (first_pivot) {
            first_pivot = false;
            std::advance(pivot, std::rand() % d);
        } else {
            std::advance(pivot, d / 2);
        }
        auto j = left;
        std::swap(*pivot, *right);
        for (auto i = left; i != right; i++) {
            if (comp(*i, *right)) {
                std::swap(*i, *j);
                j++;
            }
        }
        std::swap(*j, *right);
        pivot = j;

        auto left_d = std::distance(left, pivot);
        auto right_d = std::distance(pivot + 1, right);
        if (left_d > right_d) {
            if (left_d > 0) {
                if (left_d > thread_limit) {
                    wg.go([impl, left, pivot]() { impl(left, pivot); });
                } else {
                    impl(left, pivot);
                }
            }
            if (right_d > 0) {
                impl(pivot + 1, right);
            }
        } else {
            if (right_d > 0) {
                if (right_d > thread_limit) {
                    wg.go([impl, pivot, right]() { impl(pivot + 1, right); });
                } else {
                    impl(pivot + 1, right);
                }
            }
            if (left_d > 0) {
                impl(left, pivot);
            }
        }
        // if (std::distance(left, pivot) > 0) {
        //     wg.go([impl, left, pivot]() { impl(left, pivot); });
        //     // impl(left, pivot);
        // }
        // if (std::distance(pivot + 1, right) > 0) {
        //     wg.go([impl, pivot, right]() { impl(pivot + 1, right); });
        //     // impl(pivot + 1, right);
        // }
    };
    impl(first, std::prev(last));
    wg.wait();
}

} // namespace goxx