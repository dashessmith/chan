
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

} // namespace goxx

#include "chan.ipp"