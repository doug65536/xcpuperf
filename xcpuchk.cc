#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <pthread.h>

std::atomic_uint odd_ready;
std::atomic_uint even_ready;
std::atomic_uint go;
std::atomic_uint x;

unsigned done_x = 3000000;

template<std::memory_order load_order, std::memory_order store_order>
void odd()
{
    odd_ready.store(1, store_order);

    while (!go.load(load_order));

    while (x < done_x) {
        unsigned n = x.load(load_order);
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

    while (x < done_x) {
        unsigned n = x.load(load_order);
        if (!(n & 1))
            x.store(n + 1, store_order);
    }

    even_ready.store(0, store_order);
}

template<std::memory_order load_order, std::memory_order store_order>
void test(unsigned cpus)
{
    cpu_set_t cpuset;

    for (unsigned i = 1; i < cpus; ++i)  {
        odd_ready.store(0, store_order);
        even_ready.store(0, store_order);

        std::thread t1(odd<load_order, store_order>);
        std::thread t2(even<load_order, store_order>);

        CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        pthread_setaffinity_np(t1.native_handle(),
                               sizeof(cpu_set_t), &cpuset);

        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(t2.native_handle(),
                               sizeof(cpu_set_t), &cpuset);

        while (!odd_ready.load(load_order));
        while (!even_ready.load(load_order));
        x.store(0, store_order);
        auto st = std::chrono::high_resolution_clock::now();

        go.store(1, store_order);

        while (even_ready.load(load_order) ||
               odd_ready.load(load_order));
        auto en = std::chrono::high_resolution_clock::now();

        std::chrono::high_resolution_clock::duration elap =
                std::chrono::duration_cast<std::chrono::nanoseconds>(en - st);

        printf("0<->%2u %3luns\n", i, elap.count() / done_x);

        t1.join();
        t2.join();

        go.store(0, store_order);
    }
}

int main()
{
    unsigned cpus = std::thread::hardware_concurrency();

    printf("testing acquire/release\n");
    test<std::memory_order_acquire, std::memory_order_release>(cpus);

    printf("testing relaxed/relaxed\n");
    test<std::memory_order_relaxed, std::memory_order_relaxed>(cpus);
}
