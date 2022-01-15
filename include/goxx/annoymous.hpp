#pragma once

#define goxx_annoy___(x, y, dash, z) x##y##dash##z
#define goxx_annoy__(x, y, dash, z) goxx_annoy___(x, y, dash, z)
#define goxx_annoy goxx_annoy__(goxx_annoy__, __LINE__, __, __COUNTER__)
