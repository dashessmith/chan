#pragma once

namespace goxx {
template <class Iterator, class Compare>
void mt_sort(Iterator first, Iterator last, Compare comp);

template <class Iterator>
void mt_sort(Iterator first, Iterator last);
} // namespace goxx

#include "goxx/mt_sort.ipp"