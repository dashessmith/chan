#pragma once
#include <memory>
#include <string>
namespace goxx {
struct getOption {
    std::string tag;
    bool permanent = false;
};

template <class Dependency, class Creator>
auto get(Creator &&creator, const getOption &option = getOption{})
    -> std::shared_ptr<std::decay_t<Dependency>>;

template <class Dependency>
auto get(const getOption &option = getOption{})
    -> std::shared_ptr<std::decay_t<Dependency>>;

} // namespace goxx
#include "goxx/get.ipp"