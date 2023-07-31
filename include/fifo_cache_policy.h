#ifndef LSMKVSTORE_FIFO_CACHE_POLICY_H_
#define LSMKVSTORE_FIFO_CACHE_POLICY_H_

#include <list>
#include <unordered_map>

#include "cache_policy.h"

namespace caches {

/**
 * @brief FIFO缓存策略，先进来的先淘汰
*/
template <typename Key>
class FIFOCachePolicy : public ICachePolicy<Key> {
public:
    using fifo_iterator = typename std::list<Key>::const_iterator; // C++20以前，模板中必须使用typename关键字

    FIFOCachePolicy() = default;
    ~FIFOCachePolicy() = default;

    void Insert(const Key &key) override {
        fifo_queue_.emplace_front(key);
        key_map_[key] = fifo_queue_.begin();
    }

    void Touch(const Key &key) noexcept override {
        // 空
    }

    void Erase(const Key &key) noexcept override {
        fifo_iterator iter = key_map_[key];
        fifo_queue_.erase(iter);
        key_map_.erase(key);
    }

    const Key &ReplCandidate() const noexcept override {
        return fifo_queue_.back();
    }

private:
    std::list<Key> fifo_queue_;
    std::unordered_map<Key, fifo_iterator> key_map_;
};

};      // namespace caches

#endif // !LSMKVSTORE_FIFO_CACHE_POLICY_H_