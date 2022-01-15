#pragma once

namespace goxx {
#define goxx_annoy___(x, y) x##y
#define goxx_annoy__(x, y) goxx_annoy___(x, y)
#define goxx_annoy goxx_annoy__(goxx_annoy__, __LINE__)
} // namespace goxx