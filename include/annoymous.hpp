#pragma once

namespace goxx {
#define goxx___(x, y) x##y
#define goxx__(x, y) goxx___(x, y)
#define goxx_ goxx__(_annoymous_, __LINE__)
} // namespace goxx