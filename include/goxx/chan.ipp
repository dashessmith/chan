
#pragma once

#include "goxx/chan.hpp"
#include <thread>

namespace goxx {

template <class T>
Chan<T>::Chan(size_t size) : buffer_(size) {}

template <class T>
Chan<T>::~Chan() {
    close();
}

template <class T>
bool Chan<T>::closed() {
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
bool Chan<T>::push(T &&t) {
    if (buffer_.empty())
        return push_unbuffered(std::move(t));
    return push_buffered(std::move(t));
}

template <class T>
bool Chan<T>::push_unbuffered(T &&t) {
    auto ul = std::unique_lock{mtx_};
    cv_.wait(ul, [&]() { return closed_ || consumer_; });
    if (closed_) {
        ul.unlock();
        cv_.notify_all();
        return false;
    }
    consumer_(std::move(t));
    consumer_ = nullptr;
    ul.unlock();
    cv_.notify_all();
    return true;
}

template <class T>
bool Chan<T>::push_buffered(T &&t) {
    auto ul = std::unique_lock{mtx_};
    cv_.wait(ul, [&]() { return closed_ || !buffer_[widx_]; });
    if (closed_) {
        ul.unlock();
        cv_.notify_all();
        return false;
    }

    buffer_[widx_] = std::move(t);
    widx_ = next_idx(widx_);
    ul.unlock();
    cv_.notify_all();
    return true;
}

template <class T>
size_t Chan<T>::next_idx(size_t idx) const {
    ++idx;
    if (idx >= buffer_.size())
        idx = 0;
    return idx;
}
template <class T>
bool Chan<T>::buffer_empty() const {
    return !buffer_[ridx_];
}

template <class T>
bool Chan<T>::try_push(T &&t) {
    if (buffer_.empty()) {
        return try_push_unbuffered(std::move(t));
    }
    return try_push_buffered(std::move(t));
}

template <class T>
bool Chan<T>::try_push_buffered(T &&t) {
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
    }
    ul.unlock();
    cv_.notify_all();
    return res;
}

template <class T>
bool Chan<T>::try_push_unbuffered(T &&t) {
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
    if (!consumer_) {
        ul.unlock();
        cv_.notify_all();
        return false;
    }
    consumer_(std::move(t));
    consumer_ = nullptr;
    ul.unlock();
    cv_.notify_all();
    return true;
}

template <class T>
std::optional<T> Chan<T>::pop() {
    if (buffer_.empty())
        return pop_unbuffered();
    return pop_buffered();
}
template <class T>
std::optional<T> Chan<T>::pop_buffered() {
    auto ul = std::unique_lock{mtx_};
    cv_.wait(ul, [&]() { return !buffer_empty() || closed_; });
    auto res = popt();
    ul.unlock();
    cv_.notify_all();
    return std::move(res);
}

template <class T>
std::optional<T> Chan<T>::pop_unbuffered() {
    auto ul = std::unique_lock{mtx_};
    cv_.wait(ul, [&]() { return !consumer_ || closed_; });
    if (closed_) {
        ul.unlock();
        cv_.notify_all();
        return std::nullopt;
    }
    std::optional<T> res;
    consumer_ = [&res](T &&t) { res = std::move(t); };
    cv_.notify_all();
    cv_.wait(ul, [&]() { return res.has_value() || closed_; });
    consumer_ = nullptr;
    ul.unlock();
    cv_.notify_all();
    return std::move(res);
}

template <class T>
std::optional<T> Chan<T>::try_pop() {
    if (buffer_.empty())
        return try_pop_unbuffered();
    return try_pop_buffered();
}

template <class T>
std::optional<T> Chan<T>::try_pop_buffered() {
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
std::optional<T> Chan<T>::try_pop_unbuffered() {
    auto ul = std::unique_lock{mtx_, std::defer_lock};
    if (!ul.try_lock()) {
        cv_.notify_all();
        return std::nullopt;
    }
    if (consumer_) {
        ul.unlock();
        cv_.notify_all();
        return std::nullopt;
    }
    std::optional<T> res;
    consumer_ = [&res](T &&t) { res = std::move(t); };
    ul.unlock();
    cv_.notify_all();
    this_thread::yield();
    ul.lock();
    consumer_ = nullptr;
    ul.unlock();
    cv_.notify_all();
    return std::move(res);
}

template <class T>
std::optional<T> Chan<T>::popt() {
    std::optional<T> res;
    res.swap(buffer_[ridx_]);
    ridx_ = next_idx(ridx_);
    return std::move(res);
}
/* range expression */

/* begin iterator */
template <class T>
typename Chan<T>::Iterator Chan<T>::begin() {
    Iterator res{*this};
    res.tmp_ = std::move(pop());
    return res;
}

/* != */
template <class T>
bool Chan<T>::Iterator::operator!=(const Chan<T>::Iterator &) const {
    return tmp_.has_value();
}

template <class T>
T Chan<T>::Iterator::operator*() {
    // goxx_defer([this]() { tmp_.reset(); });
    return std::move(*tmp_);
}

/* iterate */
template <class T>
void Chan<T>::Iterator::operator++() {
    tmp_ = std::move(c_.pop());
}

/* construct iterator */
template <class T>
Chan<T>::Iterator::Iterator(Chan<T> &c) : c_(c) {}

/* end iterator */
template <class T>
typename Chan<T>::Iterator Chan<T>::end() {
    return Iterator{*this};
}

} // namespace goxx
