#pragma once
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

    std::function<void(Iterator, Iterator)> impl = [&](auto left, auto right) {
        if (std::distance(left, right) <= 0) {
            return;
        }
        auto pivot = left;
        if (first_pivot) {
            first_pivot = false;
            std::advance(pivot, std::rand() % N);
        } else {
            std::advance(pivot, std::distance(left, right) / 2);
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
        if (std::distance(left, pivot) >= 1) {
            impl(left, pivot);
        }
        if (std::distance(pivot + 1, right) >= 1) {
            impl(pivot + 1, right);
        }
    };
    impl(first, std::prev(last));
}

} // namespace goxx