#pragma once

namespace goxx {

// helper type for the visitor #4
template <class... Ts>
struct cases : Ts... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
cases(Ts...) -> cases<Ts...>;

} // namespace goxx