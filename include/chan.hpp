
#pragma once

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
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
    // if size == 0 , it becomes synchronous channel
    Chan(size_t size = 0);

    ~Chan(); // calls close

    void close();     // set closed_
    bool is_closed(); // check closed_
    bool empty();     // check l_.empty && closed_
    operator bool();  // check !(l_.empty && closed_)

    bool push(const T &t);
    bool push(T &&t);

    bool operator<<(const T &t);
    bool operator<<(T &&t);

    bool try_push(const T &t);
    bool try_push(T &&t);

    std::optional<T> pop();
    std::optional<T> try_pop();

    /***************************************
         range expression
          for (auto x : ch) {

                  }
     ********************************/

    class Iterator {
        friend class Chan;

      public:
        inline bool operator!=(const Iterator &end);
        inline void operator++();
        inline T operator*();

      private:
        inline Iterator(Chan<T> &);
        Chan<T> &c_;
        std::optional<T> tmp_;
    };
    Iterator begin();
    Iterator end();

  private:
    void _ntfclose();
    bool _push(T &&t);
    bool _try_push(T &&t);
    bool closed_ = false;
    std::mutex wmtx_;
    // std::mutex wmtx_;
    std::condition_variable wcv_;
    std::condition_variable rcv_;
    std::condition_variable receiver_wcv_;
    std::function<T && ()> receiver_ = nullptr;
    // std::function<T && ()> receiver_ = nullptr;
    std::vector<std::optional<T>> v_;
    size_t readpos_ = 0;
    size_t writepos_ = 0;
};

template <class T>
Chan<T>::Chan(size_t size) {
    v_ = std::vector<std::optional<T>>(size);
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
bool Chan<T>::empty() {
    std::unique_lock<std::mutex> _(wmtx_);
    return closed_ && receiver_ == nullptr && l_.empty();
}

template <class T>
Chan<T>::operator bool() {
    return !empty();
}

template <class T>
void Chan<T>::close() {
    closed_ = true;
    _ntfclose();
}

template <class T>
bool Chan<T>::push(const T &t) {
    return _push(std::move(T{t}));
}
template <class T>
bool Chan<T>::push(T &&t) {
    return _push(std::move(t));
}

template <class T>
bool Chan<T>::_push(T &&t) {
    if (v_.empty()) {
        std::unique_lock<std::mutex> ul{wmtx_};
        wcv_.wait(ul, [this]() { return (closed_ || !receiver_); });
        if (closed_) {
            ul.unlock();
            _ntfclose();
            return false;
        }
        receiver_ = [t = std::move(t)]() mutable -> T && {
            return std::move(t);
        };

        rcv_.notify_one();
        receiver_wcv_.wait(ul, [this]() { return (closed_ || !receiver_); });

        // auto trytimes = 0;
        // do {
        //     rcv_.notify_one();
        //     trytimes++;
        //     assert(trytimes <= 100);
        //     // std::cout << "try " << trytimes << "\n";
        // } while (!receiver_wcv_.wait_for(ul, std::chrono::seconds(1),
        // [this]() {
        //     return (closed_ || !receiver_);
        // }));
        // if (trytimes > 1) {
        //     std::cout << "try times " << trytimes << "\n";
        // }

        if (receiver_) {
            receiver_ = nullptr;
            ul.unlock();
            _ntfclose();
            // std::cout << "producer detect close\n";
            return false;
        }
        ul.unlock();
        wcv_.notify_one();
        return true;
    }
    std::unique_lock<std::mutex> ul{wmtx_};
    wcv_.wait(ul, [this]() { return closed_ || !v_[writepos_]; });
    if (closed_) {
        ul.unlock();
        _ntfclose();
        return false;
    }
    v_[writepos_].emplace(std::move(t));
    writepos_ = (writepos_ + 1) % v_.size();
    auto ntfw = !v_[writepos_];
    ul.unlock();
    rcv_.notify_one();
    if (ntfw) {
        wcv_.notify_one();
    }
    return true;
}

template <class T>
bool Chan<T>::try_push(const T &t) {
    return _try_push(t);
}

template <class T>
bool Chan<T>::try_push(T &&t) {
    return _try_push(std::move(t));
}
template <class T>
bool Chan<T>::_try_push(T &&t) {
    std::unique_lock<std::mutex> ul(wmtx_, std::defer_lock);
    if (!ul.try_lock()) {
        return false;
    }
    if (closed_) {
        ul.unlock();
        _ntfclose();
        return false;
    }
    if (v_.empty()) {
        if (receiver_)
            return false;

        receiver_ = [t = std::move(t)]() mutable -> T && {
            return std::move(t);
        };
        rcv_.notify_one();
        receiver_wcv_.wait(ul, [this]() { return (closed_ || !receiver_); });
        if (receiver_) {
            receiver_ = nullptr;
            ul.unlock();
            _ntfclose();
            return false;
        }
        ul.unlock();
        wcv_.notify_one();
        return true;
    }
    if (v_[writepos_])
        return false;
    v_[writepos_].emplace(std::move(t));
    writepos_ = (writepos_ + 1) % v_.size();
    auto ntfw = !v_[writepos_];
    ul.unlock();
    rcv_.notify_one();
    if (ntfw)
        wcv_.notify_one();
    return true;
}
template <class T>
bool Chan<T>::operator<<(const T &t) {
    return _push(t);
}
template <class T>
bool Chan<T>::operator<<(T &&t) {
    return _push(std::move(t));
}

template <class T>
std::optional<T> Chan<T>::pop() {
    if (v_.empty()) {
        receiver_wcv_.notify_one();
        std::unique_lock<std::mutex> ul{wmtx_};
        rcv_.wait(ul, [this]() mutable { return closed_ || receiver_; });
        // std::cout << "consumer wake up\n";
        if (receiver_) {
            std::optional<T> ret{std::move(receiver_())};
            receiver_ = nullptr;
            ul.unlock();
            receiver_wcv_.notify_one();
            return std::move(ret);
        }
        // std::cout << "closed ? " << closed_ << ", receiver ?"
        //           << (receiver_ != nullptr) << "\n";
        ul.unlock();
        _ntfclose();
        return std::optional<T>{};
    }
    std::unique_lock<std::mutex> ul{wmtx_};
    rcv_.wait(ul, [this]() { return closed_ || v_[readpos_]; });

    if (v_[readpos_]) {
        std::optional<T> ret;
        swap(ret, std::move(v_[readpos_]));
        readpos_ = (readpos_ + 1) % v_.size();
        auto ntfr = v_[readpos_];
        ul.unlock();
        if (ntfr) {
            rcv_.notify_one();
        }
        wcv_.notify_one();
        return std::move(ret);
    }
    ul.unlock();
    _ntfclose();
    return std::optional<T>{};
}

template <class T>
std::optional<T> Chan<T>::try_pop() {
    if (v_.empty()) {
        std::unique_lock<std::mutex> ul{wmtx_, std::defer_lock};
        if (!ul.try_lock)
            return false;
        receiver_wcv_.notify_one();
        if (receiver_) {
            std::optional<T> ret{std::move(*receiver_)};
            receiver_called_ = true;
            ul.unlock();
            receiver_wcv_.notify_one();
            return std::move(ret);
        }
        if (closed_) {
            ul.unlock();
            _ntfclose();
            return std::optional<T>{};
        }
        return std::optional<T>{};
    }
    std::unique_lock<std::mutex> ul{wmtx_, std::defer_lock};
    if (!ul.try_lock)
        return false;
    if (v_[readpos_]) {
        std::optional<T> ret;
        swap(ret, std::move(v_[readpos_]));
        readpos_ = (readpos_ + 1) % v_.size();
        auto ntfr = v_[readpos_];
        ul.unlock();
        if (ntfr) {
            rcv_.notify_one();
        }
        wcv_.notify_one();
        return std::move(ret);
    }
    if (closed_) {
        ul.unlock();
        _ntfclose();
        return std::optional<T>{};
    }
    return std::optional<T>{};
}

template <class T>
void Chan<T>::_ntfclose() {
    wcv_.notify_one();
    rcv_.notify_one();
    receiver_wcv_.notify_one();
}
/* range expression */

/* != */
template <class T>
bool Chan<T>::Iterator::operator!=(const Chan<T>::Iterator &_) {
    this->tmp_ = this->c_.pop();
    return this->tmp_.has_value();
}

/* iterate */
template <class T>
void Chan<T>::Iterator::operator++() {}

template <class T>
T Chan<T>::Iterator::operator*() {
    std::optional<T> tmp;
    swap(tmp, tmp_);
    return std::move(*tmp);
}
/* construct iterator */
template <class T>
Chan<T>::Iterator::Iterator(Chan<T> &c) : c_(c) {}

/* begin iterator */
template <class T>
typename Chan<T>::Iterator Chan<T>::begin() {
    return Iterator(*this);
}

/* end iterator */
template <class T>
typename Chan<T>::Iterator Chan<T>::end() {
    return Iterator(*this);
}

} // namespace goxx
