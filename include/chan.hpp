
#pragma once

#include "defer.hpp"
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <optional>
#include <queue>
#include <unordered_set>
#include <utility>

namespace goxx {
/*
        GO style chan

        *NOTE* can use as range expression
 */
template <class T>
class Chan {
  public:
    /******************************
          regular interface

     **************************************/
    // if size == 0 , it becomes synchronous unbuffered channel
    inline Chan(size_t size = 0);

    inline ~Chan(); // calls close

    inline void close();     // set closed_
    inline bool is_closed(); // check closed_
    inline bool empty();     // check l_.empty && closed_
    inline operator bool();  // check !(l_.empty && closed_)

    inline bool push(const T &t);
    inline bool push(T &&t);

    inline bool operator<<(const T &t);
    inline bool operator<<(T &&t);

    inline bool try_push(const T &t);
    inline bool try_push(T &&t);

    inline std::optional<T> pop();
    inline std::optional<T> try_pop();

    /***************************************
         range expression
          for (auto x : ch) {

                  }
     ********************************/

    class Iterator {
        friend class Chan;

      public:
        inline bool operator!=(const Iterator &end) const;
        inline void operator++();
        inline T operator*();

      private:
        inline Iterator(Chan<T> &);
        Chan<T> &c_;
        std::optional<T> tmp_;
    };
    inline Iterator begin();
    inline Iterator end();

  private:
    struct Consumer_ {
        // friend class std::deque<Consumer_ *>;
        std::optional<T> t_;
        std::condition_variable cv_;
    };
    inline void ntfclose_();
    inline Consumer_ *popc_();
    inline std::optional<T> popb_();
    bool closed_ = false;
    std::mutex wmtx_;
    std::condition_variable wcv_;
    std::deque<Consumer_ *> consumers_;
    std::queue<T> buffer_;
    size_t buffer_size_ = 0;
};

template <class T>
Chan<T>::Chan(size_t size) : buffer_size_(size) {}

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
    std::unique_lock<std::mutex> _(wmtx_);
    return closed_ && !ub_ && l_.empty();
}

template <class T>
Chan<T>::operator bool() {
    return !empty();
}

template <class T>
void Chan<T>::close() {
    closed_ = true;
    ntfclose_();
}

template <class T>
bool Chan<T>::push(const T &t) {
    std::unique_lock<std::mutex> ul(wmtx_);
    wcv_.wait(ul, [this]() {
        return closed_ || !consumers_.empty() || buffer_.size() < buffer_size_;
    });
    if (closed_) {
        ntfclose_();
        return false;
    }
    if (auto c = popc_(); c) {
        c->t_.emplace(t);
        c->cv_.notify_one();
        return true;
    }
    buffer_.emplace(t);
    wcv_.notify_one();
    return true;
}

template <class T>
bool Chan<T>::push(T &&t) {
    std::unique_lock<std::mutex> ul(wmtx_);
    wcv_.wait(ul, [this]() {
        return closed_ || !consumers_.empty() || buffer_.size() < buffer_size_;
    });
    if (closed_) {
        ntfclose_();
        return false;
    }
    if (auto c = popc_(); c) {
        c->t_.emplace(std::move(t));
        c->cv_.notify_one();
        wcv_.notify_one();
        return true;
    }
    buffer_.emplace(std::move(t));
    wcv_.notify_one();
    return true;
}

template <class T>
bool Chan<T>::try_push(const T &t) {
    std::unique_lock<std::mutex> ul(wmtx_, std::defer_lock);
    if (!ul.try_lock())
        return false;
    if (closed_) {
        ntfclose_();
        return false;
    }
    if (auto c = popc_(); c) {
        c->t_.set_value(t);
        c->cv_.notify_one();
        wcv_.notify_one();
        return true;
    }
    if (buffer_.size() < buffer_size_) {
        buffer_.push(t);
        wcv_.notify_one();
        return true;
    }
    wcv_.notify_one();
    return false;
}

template <class T>
bool Chan<T>::try_push(T &&t) {
    std::unique_lock<std::mutex> ul(wmtx_, std::defer_lock);
    if (!ul.try_lock())
        return false;
    if (closed_) {
        ntfclose_();
        return false;
    }
    if (auto c = popc_(); c) {
        c->t_.set_value(std::move(t));
        c->cv_.notify_one();
        return true;
    }
    if (buffer_.size() < buffer_size_) {
        buffer_.push(std::move(t));
        wcv_.notify_one();
        return true;
    }
    return false;
}

template <class T>
bool Chan<T>::operator<<(const T &t) {
    return push(t);
}
template <class T>
bool Chan<T>::operator<<(T &&t) {
    return push(std::move(t));
}

template <class T>
std::optional<T> Chan<T>::pop() {
    wcv_.notify_one();
    std::unique_lock<std::mutex> ul(wmtx_);
    if (!buffer_.empty()) {
        wcv_.notify_one();
        return popb_();
    }
    Consumer_ c;
    consumers_.push_back(&c);
    c.cv_.wait(ul, [this, &c]() { return closed_ || c.t_; });
    if (c.t_) {
        wcv_.notify_one();
        return c.t_;
    }
    if (auto itr = std::find(consumers_.begin(), consumers_.end(), &c);
        itr != consumers_.end()) {
        consumers_.erase(itr);
    }
    ntfclose_();
    return std::nullopt;
}

template <class T>
std::optional<T> Chan<T>::try_pop() {
    wcv_.notify_one();
    std::unique_lock<std::mutex> ul(wmtx_, std::defer_lock);
    if (!ul.try_lock()) {
        wcv_.notify_one();
        return std::nullopt;
    }
    if (!buffer_.empty()) {
        wcv_.notify_one();
        return popb_();
    }
    Consumer_ c;
    consumers_.push_back(&c);
    ul.unlock();
    wcv_.notify_one();
    std::this_thread::yield();
    ul.lock();
    if (c.t_) {
        wcv_.notify_one();
        return c.t_;
    }
    if (auto itr = std::find(consumers_.begin(), consumers_.end(), &c);
        itr != consumers_.end()) {
        consumers_.erase(itr);
    }
    if (c.closed_) {
        ntfclose_();
    } else {
        wcv_.notify_one();
    }
    return std::nullopt;
}

template <class T>
void Chan<T>::ntfclose_() {
    wcv_.notify_one();
    for (auto c : consumers_) {
        c->cv_.notify_one();
        break;
    }
}

template <class T>
typename Chan<T>::Consumer_ *Chan<T>::popc_() {
    typename Chan<T>::Consumer_ *res = nullptr;
    if (!consumers_.empty()) {
        res = consumers_.front();
        consumers_.pop_front();
    }
    return res;
}

template <class T>
std::optional<T> Chan<T>::popb_() {
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
    Defer _{[this]() { tmp_.reset(); }};
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
