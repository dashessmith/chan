
#pragma once

#include "goxx/defer.hpp"
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
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
    Chan(size_t size);

    ~Chan(); // calls close

    void close();     // set closed_
    bool closed();    // check closed_
    bool exhausted(); // check l_.empty && closed_
    operator bool();  // check !(l_.empty && closed_)

    bool push(T &&t);
    bool try_push(T &&t);
    std::optional<T> pop();
    std::optional<T> try_pop();

    /***************************************
         range expression
          for (auto x : ch) { }
     ********************************/

    class Iterator {
        friend class Chan;

      public:
        bool operator!=(const Iterator &end) const;
        void operator++();
        T operator*();

      private:
        Iterator(Chan<T> &);
        Chan<T> &c_;
        std::optional<T> tmp_;
    };
    Iterator begin();
    Iterator end();

  private:
    bool push_buffered(T &&t);
    bool push_unbuffered(T &&t);
    bool try_push_buffered(T &&t);
    bool try_push_unbuffered(T &&t);

    std::optional<T> pop_buffered();
    std::optional<T> pop_unbuffered();
    std::optional<T> try_pop_buffered();
    std::optional<T> try_pop_unbuffered();

    size_t next_idx(size_t idx) const;
    bool buffer_empty() const;
    bool closed_ = false;
    std::optional<T> popt();
    std::mutex mtx_;
    std::condition_variable cv_;
    std::vector<std::optional<T>> buffer_;
    std::function<void(T &&)> consumer_;
    size_t ridx_ = 0;
    size_t widx_ = 0;
};

} // namespace goxx

#include "goxx/chan.ipp"