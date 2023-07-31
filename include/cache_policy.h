#ifndef LSMKVSTORE_CACHE_POLICY_H_
#define LSMKVSTORE_CACHE_POLICY_H_

namespace caches {

/**
 * @brief 缓存策略抽象基类
 */
template <typename Key>
class ICachePolicy {
public:
    virtual ~ICachePolicy() = default;

    // 插入元素
    virtual void Insert(const Key &key) = 0;

    // 访问元素
    virtual void Touch(const Key &key) = 0;

    // 删除元素
    virtual void Erase(const Key &key) = 0;

    // 返回一个根据选择的策略应当被淘汰的元素
    virtual const Key &ReplCandidate() const = 0;
};

} // namespace caches

#endif // !LSMKVSTORE_CACHE_POLICY_H_