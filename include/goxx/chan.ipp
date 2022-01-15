
#pragma once

#include "goxx/chan.hpp"
#include <thread>

namespace goxx {

template <class T>
Chan<T>::Chan(size_t size) : buffer_size_{std::max(size_t{1}, size)} {
    buffer_.resize(buffer_size_);
}

template <class T>
Chan<T>::~Chan() {
    close();
}

template <class T>
bool Chan<T>::is_closed() {
    return closed_;
}

template <class T>
bool Chan<T>::exhausted() {
    auto ul = std::unique_lock{mtx_};
    auto res = closed_ && buffer_empty();
    ul.unlock();
    return res;
}

template <class T>
Chan<T>::operator bool() {
    return !exhausted();
}

template <class T>
void Chan<T>::close() {
    closed_ = true;
    cv_.notify_all();
}

template <class T>
bool Chan<T>::push(const T &t) {
    return push(std::move(T{t}));
}

template <class T>
bool Chan<T>::push(T &&t) {
    auto ul = std::unique_lock{mtx_};
    cv_.wait(ul, [&]() {
        return closed_ || !buffer_[widx_] || !buffer_[next_idx(widx_)];
    });
    if (closed_) {
        ul.unlock();
        cv_.notify_all();
        return false;
    }
    if (!buffer_[widx_]) {
        buffer_[widx_] = std::move(t);
    } else {
        widx_ = next_idx(widx_);
        buffer_[widx_] = std::move(t);
    }
    widx_ = next_idx(widx_);
    ul.unlock();
    cv_.notify_all();
    return true;
}

template <class T>
size_t Chan<T>::next_idx(size_t idx) const {
    ++idx;
    if (idx >= buffer_size_)
        idx = 0;
    return idx;
}
template <class T>
bool Chan<T>::buffer_empty() const {
    return !buffer_[ridx_];
}

template <class T>
bool Chan<T>::try_push(const T &t) {
    return try_push(std::move(T{t}));
}

template <class T>
bool Chan<T>::try_push(T &&t) {
    auto ul = std::unique_lock{mtx_, std::defer_lock};
    if (!ul.try_lock()) {
        cv_.notify_all();
        return false;
    }
    if (closed_) {
        ul.unlock();
        cv_.notify_all();
        return false;
    }
    auto res = false;
    if (!buffer_[widx_]) {
        buffer_[widx_] = std::move(t);
        widx_ = next_idx(widx_);
        res = true;
    } else if (next = next_idx(widx_); !buffer[next]) {
        buffer_[next] = std::move(t);
		widx_ = next_idx(next);
        res = true;
    }
    ul.unlock();
    ul.notify_all();
    return res;
}

template <class T>
std::optional<T> Chan<T>::pop() {
    auto ul = std::unique_lock{mtx_};
    cv_.wait(ul, [&]() { return !buffer_empty() || closed_; });
    auto res = popt();
    ul.unlock();
    cv_.notify_all();
    return std::move(res);
}

template <class T>
std::optional<T> Chan<T>::try_pop() {
    auto ul = std::unique_lock{mtx_, std::defer_lock};
    if (!ul.try_lock()) {
        cv_.notify_all();
        return std::nullopt;
    }
    auto res = popt();
    ul.unlock();
    cv_.notify_all();
    return std::move(res);
}

template <class T>
std::optional<T> Chan<T>::popt() {
    std::optional<T> res{std::move(buffer_[ridx_])};
    buffer_[ridx_].reset();
    ridx_ = next_idx(ridx_);
    return res;
}
/* range expression */

/* != */
template <class T>
bool Chan<T>::Iterator::operator!=(const Chan<T>::Iterator &) const {
    return tmp_.has_value();
}

/* iterate */
template <class T>
void Chan<T>::Iterator::operator++() {
    tmp_ = c_.pop();
}

template <class T>
T Chan<T>::Iterator::operator*() {
    goxx_defer([this]() { tmp_.reset(); });
    return std::move(*tmp_);
}
/* construct iterator */
template <class T>
Chan<T>::Iterator::Iterator(Chan<T> &c) : c_(c) {}

/* begin iterator */
template <class T>
typename Chan<T>::Iterator Chan<T>::begin() {
    Iterator res{*this};
    res.tmp_ = pop();
    return res;
}

/* end iterator */
template <class T>
typename Chan<T>::Iterator Chan<T>::end() {
    return Iterator{*this};
}

} // namespace goxx
