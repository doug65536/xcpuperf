#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <pthread.h>

std::atomic_uint odd_ready;
std::atomic_uint even_ready;
std::atomic_uint go;
std::atomic_uint x;

using test_duration = std::chrono::high_resolution_clock::duration;
using test_clock = std::chrono::high_resolution_clock;

unsigned done_x = 3000000;

template<std::memory_order load_order, std::memory_order store_order>
void odd()
{
    odd_ready.store(1, store_order);

    while (!go.load(load_order));

    unsigned n;
    while ((n = x.load(load_order)) < done_x) {
        if (n & 1)
            x.store(n + 1, store_order);
    }

    odd_ready.store(0, store_order);
}

template<std::memory_order load_order, std::memory_order store_order>
void even()
{
    even_ready.store(1, store_order);

    while (!go.load(load_order));

    unsigned n;
    while ((n = x.load(load_order)) < done_x) {
        if (!(n & 1))
            x.store(n + 1, store_order);
    }

    even_ready.store(0, store_order);
}

template<std::memory_order load_order, std::memory_order store_order>
void test(unsigned cpus)
{
    cpu_set_t cpuset;

    for (unsigned c1 = 0; c1 < cpus; ++c1)  {
        for (unsigned c2 = 0; c2 < cpus; ++c2)  {
            if (c1 != c2) {
                test_duration best;
                done_x = 300000;
                best = test_duration::max();

                size_t iter_cnt = 100;

                for (size_t iter = 0; iter < iter_cnt; ++iter) {
                    odd_ready.store(0, store_order);
                    even_ready.store(0, store_order);

                    std::thread t1(odd<load_order, store_order>);
                    std::thread t2(even<load_order, store_order>);

                    CPU_ZERO(&cpuset);
                    CPU_SET(c1, &cpuset);
                    pthread_setaffinity_np(t1.native_handle(),
                                           sizeof(cpu_set_t), &cpuset);

                    CPU_ZERO(&cpuset);
                    CPU_SET(c2, &cpuset);
                    pthread_setaffinity_np(t2.native_handle(),
                                           sizeof(cpu_set_t), &cpuset);

                    while (!odd_ready.load(load_order));
                    while (!even_ready.load(load_order));

                    x.store(0, store_order);
                    auto st = test_clock::now();

                    go.store(1, store_order);

                    while (even_ready.load(load_order) ||
                           odd_ready.load(load_order));

                    auto en = test_clock::now();

                    test_duration elap =
                            std::chrono::duration_cast<
                            std::chrono::nanoseconds>(en - st);

                    uint64_t c = elap.count();
                    bool was_better = c < 1000000;

                    unsigned new_done_x = (__int128)done_x * 1000000 / c;

                    if (new_done_x < done_x) {
                        // Recalibrate
                        done_x = new_done_x;

                        // Start over
                        iter = -1;
                    } else if (iter > 0 && best > elap) {
                        best = elap;
                    }

                    if (iter == (iter_cnt - 1)) {
                        if (c2 == 0)
                            printf("\n%2u", c1);

                        printf(" %3lu", best.count() / done_x);
                    }

                    t1.join();
                    t2.join();

                    go.store(0, store_order);
                }
            } else {
                if (c2 == 0)
                    printf("\n%2u", c1);

                printf(" %3s", "---");
            }

            fflush(stdout);
        }
    }

    puts("");
}

int main()
{
    unsigned cpus = std::thread::hardware_concurrency();

    printf("testing acquire/release\n");
    test<std::memory_order_acquire, std::memory_order_release>(cpus);

// same result
//    printf("testing relaxed/relaxed\n");
//    test<std::memory_order_relaxed, std::memory_order_relaxed>(cpus);
}
