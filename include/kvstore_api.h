#ifndef LSMKVSTORE_API_H_
#define LSMKVSTORE_API_H_

#include <string>
#include <cstdint>

/**
 * @brief 存储引擎向外部提供的API类
 * @details 抽象类，需要其子类(具体的存储引擎实现类)实现Get、Put、Del、Reset接口
 */
class KVStoreAPI {
public:
    /**
     * @brief 构造KVStoreAPI对象
     * @param[in] dir SSTable文件存放的目录，包含n+1个子目录
     */
    KVStoreAPI(const std::string &dir) {}

    KVStoreAPI() = delete;  // 删除默认构造函数

    /**
     * @brief 插入键值对或更新键值对
     * @param[in] key 键
     * @param[in] val 值
     * @param[in] to_cache 是否缓存该元素<key,val>
     */
    virtual void Put(uint64_t key, const std::string &val, bool to_cache = false) = 0;

    /**
     * @brief 查找键值对
     * @param[in] key 要查找的key
     * @return std::string 返回key对应的val，如果key不存在，返回""
     */
    virtual std::string Get(uint64_t key) = 0;

    /**
     * @brief 删除键值对
     * @param[in] key 要删除键值对的key
     * @return true：key存在，删除成功；false：key不存在，删除失败
     */
    virtual bool Del(uint64_t key, bool to_cache = false) = 0;

    /**
     * @brief 重置kvstore
     * @details 移除所有键值对元素，包括Memtable、Immutable Memtable和所有SSTable文件
     */
    virtual void Reset() = 0;
};

#endif // !LSMKVSTORE_API_H_