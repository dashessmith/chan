#pragma once
#include <memory>
#include <string>
namespace goxx {
struct GetOption {
    std::string tag;
    bool permanent = false;
};

template <class Dependency, class Creator,
          class Storagetype = Dependency>
auto get(Creator &&creator, const GetOption &option = GetOption{})
    -> std::shared_ptr<Storagetype>;

template <class Dependency, class Storagetype = Dependency>
auto get(const GetOption &option = GetOption{}) -> std::shared_ptr<Storagetype>;

} // namespace goxx
#include "goxx/get.ipp"