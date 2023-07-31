#include <assert.h>
#include <iostream>

#include "cache.h"
#include "fifo_cache_policy.h"
#include "lru_cache_policy.h"
#include "lfu_cache_policy.h"

template <typename K, typename V>
using fifo_cache_t = typename caches::FixedSizeCache<K, V, caches::FIFOCachePolicy>;
template <typename K, typename V>
using lru_cache_t = typename caches::FixedSizeCache<K, V, caches::LRUCachePolicy>;
template <typename K, typename V>
using lfu_cache_t = typename caches::FixedSizeCache<K, V, caches::LFUCachePolicy>;

void TestFIFO() {
    fifo_cache_t<int, int> fc(2);
    fc.Put(1, 10);
    fc.Put(2, 20);
    assert(fc.Cached(1) == true);
    assert(fc.Cached(2) == true);
    std::cout << "fc.Get(1) = " <<  fc.Get(1) << std::endl;
    fc.Put(3, 30);
    assert(fc.Cached(1) == false);
    assert(fc.Cached(2) == true);
    assert(fc.Cached(3) == true);
}

void TestLRU() {
    lru_cache_t<int, int> lruc(2);
    lruc.Put(1, 10);
    lruc.Put(2, 20);
    assert(lruc.Cached(1) == true);
    assert(lruc.Cached(2) == true);
    std::cout << "lruc.Get(1) = " << lruc.Get(1) << std::endl;
    lruc.Put(3, 30);
    assert(lruc.Cached(1) == true);
    assert(lruc.Cached(2) == false);
    assert(lruc.Cached(3) == true);
}

void TestLFU() {
    lfu_cache_t<int, int> lfuc(2);
    lfuc.Put(0, 0);
    lfuc.Put(1, 10);
    lfuc.Put(2, 20);
    assert(lfuc.Cached(1) == true);
    assert(lfuc.Cached(2) == true);
    std::cout << "lfuc.Get(1) = " << lfuc.Get(1) << std::endl;
    std::cout << "lfuc.Get(1) = " << lfuc.Get(1) << std::endl;
    std::cout << "lfuc.Get(2) = " << lfuc.Get(2) << std::endl;
    lfuc.Put(3, 30);
    assert(lfuc.Cached(1) == true);
    assert(lfuc.Cached(2) == false);
    assert(lfuc.Cached(3) == true);
}

int main() {
    TestFIFO();
    TestLRU();
    TestLFU();

    return 0;
}