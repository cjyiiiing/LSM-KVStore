#ifndef LSMSTORE_NO_CACHE_POLICY_H_
#define LSMSTORE_NO_CACHE_POLICY_H_

#include <unordered_set>

#include "cache_policy.h"

namespace caches {

/**
 * @brief 无缓存策略，淘汰底层容器的第一个元素
 */
template <typename Key>
class NoCachePolicy : public ICachePolicy<Key> {
public:
    NoCachePolicy() = default;
    ~NoCachePolicy() noexcept override = default;

    void Insert(const Key &key) override {
        key_storage_.emplace(key);
    }

    void Touch(const Key &key) noexcept override {
        // 空
    }

    void Erase(const Key &key) noexcept override {
        key_storage_.erase(key);
    }

    // 返回候选置换元素的key，因为没有具体的缓存策略，所以随便返回一个
    const Key &ReplCandidate() const noexcept override {
        return *key_storage_.cbegin();
    }

private:
    std::unordered_set<Key> key_storage_;
};

}  // namespace caches

#endif // !LSMSTORE_NO_CACHE_POLICY_H_