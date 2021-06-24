#include "chan.hpp"
#include "wait_group.hpp"
#include <cstdio>

int main() {
    Chan<int> c{10};

    WaitGroup wg{};

    wg.go([&c]() {
        for (auto i = 0; i < 1000; i++) {
            c.push(i);
            printf("pushed %d\n", i);
        }
        c.close();
        printf("after close c, c is end ? %d\n", c.is_end());
    });

    wg.together([&c](size_t, size_t) {
        for (auto n : c) {
            printf("n = %d\n", n);
        }
    });
    wg.wait();
    printf("wait group done\n");
    return 0;
}