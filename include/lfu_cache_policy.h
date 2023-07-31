#ifndef LSMKVSTORE_LFU_CACHE_POLICY_H_
#define LSMKVSTORE_LFU_CACHE_POLICY_H_

#include <map>
#include <unordered_map>
#include <cstddef>

#include "cache_policy.h"

namespace caches {

    /**
     * @brief LFU缓存策略，最近最少使用的先淘汰
     */
    template <typename Key>
    class LFUCachePolicy : public ICachePolicy<Key> {
    public:
        using lfu_iterator = typename std::multimap<std::size_t, Key>::iterator;

        LFUCachePolicy() = default;
        ~LFUCachePolicy() = default;

        void Insert(const Key &key) override {
            constexpr std::size_t INIT_VAL = 1;
            key_map_[key] = lfu_cnt_map_.emplace_hint(lfu_cnt_map_.cend(), INIT_VAL, key); // 在指定位置插入
        }

        void Touch(const Key &key) override {
            auto iter = key_map_[key];
            lfu_cnt_map_.erase(iter);
            key_map_[key] = lfu_cnt_map_.emplace_hint(lfu_cnt_map_.cend(), iter->first + 1, iter->second);
        }

        void Erase(const Key &key) noexcept override {
            lfu_cnt_map_.erase(key_map_[key]);
            key_map_.erase(key);
        }

        const Key &ReplCandidate() const noexcept override {
            return lfu_cnt_map_.cbegin()->second;
        }

    private:
        std::multimap<std::size_t, Key> lfu_cnt_map_;
        std::unordered_map<Key, lfu_iterator> key_map_;
    };

}; // namespace caches

#endif // !LSMKVSTORE_LFU_CACHE_POLICY_H_