
#pragma once

#include "goxx/defer.hpp"
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
    Chan(size_t size = 0);

    ~Chan(); // calls close

    void close();     // set closed_
    bool is_closed(); // check closed_
    bool empty();     // check l_.empty && closed_
    operator bool();  // check !(l_.empty && closed_)

    bool push(const T &t);
    bool push(T &&t);

    bool try_push(const T &t);
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
    bool closed_ = false;
    std::optional<T> popt();
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<T> buffer_;
    size_t buffer_size_ = 0;
};

} // namespace goxx

#include "goxx/chan.ipp"