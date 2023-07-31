#ifndef LSMKVSTORE_CACHE_H_
#define LSMKVSTORE_CACHE_H_

#include <unordered_map>
#include <mutex>
#include <functional>  // funciton/bind
#include <algorithm>  // for_each

#include "cache_policy.h"
#include "no_cache_policy.h"

namespace caches {

/**
 * @brief 固定长度的通用缓存器，该缓存器能使用不同缓存策略：lru、lfu、fifo
 * @tparam Key 键
 * @tparam Value 值
 * @tparam Policy 缓存策略
 */
template <typename Key, typename Value, template <typename> class Policy = NoCachePolicy>
class FixedSizeCache {
public:
    using iterator = typename std::unordered_map<Key, Value>::iterator;
    using const_iterator = typename std::unordered_map<Key, Value>::const_iterator;
    using mutex_guard = typename std::lock_guard<std::mutex>;

    /**
     * @brief 构造函数
     * @param capacity cache的容量
     * @param policy 缓存策略
     */
    explicit FixedSizeCache(std::size_t capacity, const Policy<Key> policy = Policy<Key>{}) :
                                                      cache_cap_(capacity), cache_policy_(policy) {
        if (capacity <= 0) {
            throw std::invalid_argument{"Size of the cache should be bigger than 0"};  // 当使用了无效的参数时，会抛出该异常。
        }
    }

    ~FixedSizeCache() noexcept {
        Clear();
    }

    /**
     * @brief 将<key,value>插入缓存
     * @param[in] key 键
     * @param[in] value 值
    */
    void Put(const Key &key, const Value &value) {
        mutex_guard lock(mutex_);

        auto iter = FindElem(key);
        if (iter == end()) {    // 没有该元素
            if (cache_items_map_.size() >= cache_cap_) {  // 超过最大容量，需淘汰一个元素
                auto del_candidate_key = cache_policy_.ReplCandidate();
                Erase(del_candidate_key);
            }
            Insert(key, value);
        } else {  // 有该元素则更新
            Update(key, value);
        }
    }

    /**
     * @brief 获取指定key对应的value
     * @param[in] key 要查找的键
    */
    const Value &Get(const Key &key) const {
        mutex_guard lock(mutex_);

        std::pair<const_iterator, bool> pair = VisitKey(key);
        if (pair.second) {  // 存在该key
            return pair.first->second;
        } else {        // 不存在该key
            throw std::range_error{"No such key in the cache"};  // 当尝试存储超出范围的值时，会抛出该异常。
        }
    }

    /**
     * @brief 判断key是否已在cache中
     * @param[in] key 要查找的键
     */
    bool Cached(const Key &key) const noexcept {
        mutex_guard lock(mutex_);
        return FindElem(key) != end();
    }

    /**
     * @brief 获得当前cache的大小
    */
    std::size_t Size() const {
        mutex_guard lock(mutex_);
        return cache_items_map_.size();
    }

    /**
     * @brief 删除指定key的元素
     * @param[in] key 要删除的键
    */
    bool Remove(const Key &key) {
        mutex_guard lock(mutex_);
        if (FindElem(key) == end()) {
            return false;
        } else {
            Erase(key);
            return true;
        }
    }

protected:
    /**
     * @brief 重置缓存器
    */
   void Clear() {
        mutex_guard lock(mutex_);

        // 清空cache_policy_
        std::for_each(begin(), end(), [&](const std::pair<Key, Value> &elem) {
            cache_policy_.Erase(elem.first);
        });

        // 清空cache_items_map_
        cache_items_map_.clear();
   }

   const_iterator begin() const noexcept {
        return cache_items_map_.cbegin();
   }

    const_iterator end() const noexcept {
        return cache_items_map_.cend();
    }

   /**
    * @brief 插入元素<key, value>
   */
    void Insert(const Key &key, const Value &value) {
        cache_policy_.Insert(key);
        cache_items_map_.emplace(key, value);
    }

    /**
     * @brief 删除指定迭代器对应的元素
    */
    void Erase(const_iterator iter) {
        cache_policy_.Erase(iter->first);
        cache_items_map_.erase(iter);
    }

    /**
     * @brief 删除指定key对应的元素
    */
    void Erase(const Key &key) {
        cache_policy_.Erase(key);
        cache_items_map_.erase(key);
    }

    /**
     * @brief 更新key,value
    */
    void Update(const Key &key, const Value &value) {
        cache_policy_.Touch(key);       // 如果是LRU或者LFU，需要调整位置或访问次数
        cache_items_map_[key] = value;  // 调整值
    }

    /**
     * @brief 根据key获取其在cache_items_map_中对应的迭代器
     */
    const_iterator FindElem(const Key &key) const {
        return cache_items_map_.find(key);
    }

    /**
     * @brief 寻找key，若存在，更新并返回，若不存在，返回
    */
    std::pair<const_iterator, bool> VisitKey(const Key &key) const {
        const_iterator iter = FindElem(key);
        if (iter != end()) {
            cache_policy_.Touch(key);
            return {iter, true};
        } else {
            return {iter, false};
        }
    }

private:
    std::unordered_map<Key, Value> cache_items_map_;    // 存储key,value的数据结构
    mutable Policy<Key> cache_policy_;                   // 缓存策略
    mutable std::mutex mutex_;         // 互斥锁
    std::size_t cache_cap_;                   // 缓存容量
};

};  // namespace caches

#endif // !LSMKVSTORE_CACHE_H_