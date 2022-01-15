#pragma once
#include "goxx/goxx.hpp"
#include <memory>
#include <shared_mutex>
#include <typeindex>
#include <unordered_map>
namespace goxx {

namespace internal {

template <class Dependency>
auto get(std::function<std::shared_ptr<std::decay_t<Dependency>>()> &&creator,
         const getOption &option) -> std::shared_ptr<std::decay_t<Dependency>> {

    using Returntype = std::decay_t<Dependency>;

    static std::unordered_map<std::string, std::weak_ptr<Returntype>>
        referrences;
    static std::unordered_map<std::string, std::shared_ptr<Returntype>>
        permanent_referrences;
    static std::shared_mutex mtx;

    {
        auto sl = std::shared_lock{mtx};

        if (auto itr = permanent_referrences.find(option.tag);
            itr != permanent_referrences.end()) {
            return itr->second;
        }

        if (!option.permanent) {
            if (auto itr = referrences.find(option.tag);
                itr != referrences.end()) {
                if (auto sptr = itr->second.lock(); sptr) {
                    return sptr;
                }
            }
        }
    }

    std::shared_ptr<Returntype> sptr;

    auto ul = std::unique_lock{mtx};

    if (auto itr = permanent_referrences.find(option.tag);
        itr != permanent_referrences.end()) {
        sptr = itr->second;
    }

    if (!sptr) {
        if (auto itr = referrences.find(option.tag); itr != referrences.end()) {
            sptr = itr->second.lock();
        }
    }

    if (!sptr) {
        sptr = creator();
    }

    if (option.permanent ||
        permanent_referrences.find(option.tag) != permanent_referrences.end()) {

        permanent_referrences[option.tag] = sptr;
        referrences.erase(option.tag);

    } else {
        referrences[option.tag] = std::weak_ptr{sptr};
    }

    return sptr;
}
} // namespace internal

template <class Dependency, class Creator>
auto get(Creator &&creator, const getOption &option)
    -> std::shared_ptr<std::decay_t<Dependency>> {
    return internal::get<Dependency>(
        [&creator]() -> std::shared_ptr<Dependency> {
            return std::shared_ptr<std::decay_t<Dependency>>{creator()};
        },
        option);
}

template <class Dependency>
auto get(const getOption &option) -> std::shared_ptr<std::decay_t<Dependency>> {
    return internal::get<Dependency>(
        []() { return make_shared<std::decay_t<Dependency>>(); }, option);
}

} // namespace goxx
