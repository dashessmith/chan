
#pragma once

#include "goxx/chan.hpp"
#include <thread>

namespace goxx {

template <class T>
Chan<T>::Chan(size_t size) : buffer_size_{std::max(size_t{1}, size)} {}

template <class T>
Chan<T>::~Chan() {
    close();
}

template <class T>
bool Chan<T>::is_closed() {
    return closed_;
}

template <class T>
bool Chan<T>::empty() {
    auto ul = std::unique_lock{mtx_};
    auto res = closed_ && buffer_.empty();
    ul.unlock();
    return res;
}

template <class T>
Chan<T>::operator bool() {
    return !empty();
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
    cv_.wait(ul, [&]() { return closed_ || buffer_.size() < buffer_size_; });
    if (closed_) {
        ul.unlock();
        cv_.notify_all();
        return false;
    }
    buffer_.emplace(std::move(t));
    ul.unlock();
    cv_.notify_all();
    return true;
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
    if (buffer_.size() < buffer_size_) {
        buffer_.emplace(std::move(t));
        res = true;
    }
    ul.unlock();
    ul.notify_all();
    return res;
}

template <class T>
std::optional<T> Chan<T>::pop() {
    auto ul = std::unique_lock{mtx_};
    cv_.wait(ul, [&]() { return !buffer_.empty() || closed_; });
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
    if (buffer_.empty())
        return std::nullopt;
    std::optional<T> res{std::move(buffer_.front())};
    buffer_.pop();
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
