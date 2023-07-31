#ifndef LSMKVSTORE_LRU_CACHE_POLICY_H_
#define LSMKVSTORE_LRU_CACHE_POLICY_H_

#include <list>
#include <unordered_map>

#include "cache_policy.h"

namespace caches {

/**
 * @brief LRU缓存策略，最近最久未使用的先淘汰
*/
template <typename Key>
class LRUCachePolicy : public ICachePolicy<Key> {
public:
    using lru_iterator = typename std::list<Key>::iterator;

    LRUCachePolicy() = default;
    ~LRUCachePolicy() = default;

    void Insert(const Key &key) override {
        lru_queue_.emplace_front(key);
        key_map_[key] = lru_queue_.begin();
    }

    void Touch(const Key &key) override {
        // 访问的元素放到队头
        lru_queue_.splice(lru_queue_.begin(), lru_queue_, key_map_[key]);
    }

    void Erase(const Key &key) noexcept override {
        lru_iterator iter = key_map_[key];
        lru_queue_.erase(iter);
        key_map_.erase(key);
    }

    const Key &ReplCandidate() const noexcept override {
        return lru_queue_.back();
    }

    private:
        std::list<Key> lru_queue_;
        std::unordered_map<Key, lru_iterator> key_map_;
    };

};  // namespace caches

#endif // !LSMKVSTORE_LRU_CACHE_POLICY_H_