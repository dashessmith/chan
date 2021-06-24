
#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <utility>

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
    // if size == 0 , it becomes block chan forever
    Chan(size_t size = 0);

    ~Chan(); // calls close

    void close();     // set closed_
    bool is_closed(); // check closed_
    bool is_end();    // check l_.empty && closed_

    bool push(const T &t);
    bool push(T &&t);

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
        inline bool operator!=(const Iterator &end) const;
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
    bool closed_ = false;
    std::mutex mtx_;
    std::condition_variable wcv_;
    std::condition_variable rcv_;
    std::queue<T> l_;
    size_t size_ = 0;
};

template <class T>
Chan<T>::Chan(size_t size) : size_(size) {}

template <class T>
Chan<T>::~Chan() {
    close();
}

template <class T>
bool Chan<T>::is_closed() {
    return closed_;
}

template <class T>
bool Chan<T>::is_end() {
    bool res = false;
    {
        std::unique_lock<std::mutex> _(mtx_);
        res = l_.empty() && closed_;
    }
    return res;
}

template <class T>
void Chan<T>::close() {
    if (!closed_) {
        std::unique_lock<std::mutex> _(mtx_);
        closed_ = true;
    }
    wcv_.notify_all();
    rcv_.notify_all();
}

template <class T>
bool Chan<T>::try_push(const T &t) {
    {
        if (!mtx_.try_lock()) {
            return false;
        }
        std::unique_lock<std::mutex> _(mtx_, std::adopt_lock);
        if (closed_) {
            rcv_.notify_all();
            wcv_.notify_all();
            return false;
        }
        if (size_ <= 0 && !l_.empty())
            return false;
        if (size_ > 0 && l_.size() >= size_)
            return false;
        l_.push(t);
    }
    rcv_.notify_one();
    if (size_ <= 0) {
        std::unique_lock<std::mutex> ul(mtx_);
        wcv_.wait(ul, [this]() { return l_.empty(); });
    }
    return true;
}

template <class T>
bool Chan<T>::push(const T &t) {
    {
        std::unique_lock<std::mutex> ul(mtx_);
        wcv_.wait(ul, [this]() {
            return closed_ || (size_ <= 0 ? l_.empty() : l_.size() < size_);
        });
        if (closed_) {
            rcv_.notify_all();
            wcv_.notify_all();
            return false;
        }
        l_.push(t);
    }
    rcv_.notify_one();
    if (size_ <= 0) {
        std::unique_lock<std::mutex> ul(mtx_);
        wcv_.wait(ul, [this]() { return l_.empty(); });
    }
    return true;
}

template <class T>
bool Chan<T>::try_push(T &&t) {
    {
        if (!mtx_.try_lock()) {
            return false;
        }
        std::unique_lock<std::mutex> _(mtx_, std::adopt_lock);
        if (closed_) {
            rcv_.notify_all();
            wcv_.notify_all();
            return false;
        }
        if (size_ <= 0 && !l_.empty())
            return false;
        if (size_ > 0 && l_.size() >= size_)
            return false;
        l_.push(std::move(t));
    }

    rcv_.notify_one();
    if (size_ <= 0) {
        std::unique_lock<std::mutex> ul(mtx_);
        wcv_.wait(ul, [this]() { return l_.empty(); });
    }
    return true;
}
template <class T>
bool Chan<T>::push(T &&t) {
    {
        std::unique_lock<std::mutex> ul(mtx_);
        wcv_.wait(ul, [this]() {
            return closed_ || (size_ <= 0 ? l_.empty() : l_.size() < size_);
        });
        if (closed_) {
            rcv_.notify_all();
            wcv_.notify_all();
            return false;
        }
        l_.push(std::move(t));
    }
    rcv_.notify_one();
    if (size_ <= 0) {
        std::unique_lock<std::mutex> ul(mtx_);
        wcv_.wait(ul, [this]() { return l_.empty(); });
    }
    return true;
}

template <class T>
std::optional<T> Chan<T>::pop() {
    std::optional<T> ret;
    bool ntfr = false;
    {
        std::unique_lock<std::mutex> ul(mtx_);
        rcv_.wait(ul, [this]() { return closed_ || !l_.empty(); });
        if (l_.empty()) {
            rcv_.notify_all();
            wcv_.notify_all();
            return ret;
        }
        ret.emplace(std::move(l_.front()));
        l_.pop();
        ntfr = !l_.empty();
    }
    if (ntfr)
        rcv_.notify_one();

    wcv_.notify_one();

    return std::move(ret);
}

template <class T>
std::optional<T> Chan<T>::try_pop() {
    std::optional<T> ret;
    bool ntfr = false;
    {
        if (!mtx_.try_lock())
            return ret;
        std::unique_lock<std::mutex> _(mtx_, std::adopt_lock);
        if (l_.empty()) {
            rcv_.notify_all();
            wcv_.notify_all();
            return ret;
        }
        ret.emplace(std::move(l_.front()));
        l_.pop();
        ntfr = !l_.empty();
    }

    if (ntfr)
        rcv_.notify_one();

    wcv_.notify_one();

    return std::move(ret);
}
/* range expression */

/* != */
template <class T>
bool Chan<T>::Iterator::operator!=(const Chan<T>::Iterator &_) const {
    auto self = const_cast<Chan<T>::Iterator *>(this);
    self->tmp_ = self->c_.pop();
    return self->tmp_.has_value();
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
