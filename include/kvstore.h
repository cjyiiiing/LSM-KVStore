#ifndef LSMKVSTORE_KVSTORE_H_
#define LSMKVSTORE_KVSTORE_H_

#include <future>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <string>
#include <queue>
#include <thread>
#include <string.h>

#include "kvstore_api.h"
#include "table_cache.h"
#include "cache.h"
#include "thread_pool.h"
#include "options.h"
#include "utils.h"
#include "skiplist.h"

#ifdef FIFO
#include "fifo_cache_policy.h"
template <typename K, typename V>
using cache_t = typename caches::FixedSizeCache<K, V, caches::FIFOCachePolicy>;
#elif defined LRU
#include "lru_cache_policy.h"
template <typename K, typename V>
using cache_t = typename caches::FixedSizeCache<K, V, caches::LRUCachePolicy>;
#elif defined LFU
#include "lfu_cache_policy.h"
template <typename K, typename V>
using cache_t = typename caches::FixedSizeCache<K, V, caches::LFUCachePolicy>;
#endif

class KVStore : public KVStoreAPI {
public:
    KVStore(const std::string &dir);
    ~KVStore();

    void Put(uint64_t key, const std::string &val, bool to_cache = true) override;
    // 将Put函数封装为任务，以便丢进线程池
    void PutTask(uint64_t key, const std::string &val, bool to_cache = true);

    std::string Get(uint64_t key) override;
    // 将Get函数封装为任务，以便丢进线程池。返回一个包含key对应val的future对象
    std::future<std::string> GetTask(uint64_t key);

    bool Del(uint64_t key, bool to_cache = true) override;
    // 将Del函数封装为任务，以便丢进线程池
    void DelTask(uint64_t key, bool to_cache = true);

    // 删除所有SST文件及文件夹
    void Reset() override;

private:
    /**
     * @brief immutable memtable ->  level0 SST文件
     * @details 会调用MajorCompaction(1)检查Level0是否需要进行MajorCompaction
     */
    void MinorCompaction();

    /**
     * @brief 如果level-1层SST文件数量超过限制，则将level-1层的SST文件与level层的SST文件合并放到level层
     * @details 采用多路归并排序
     * @param[in] level 检查level-1层是否要进行compaction
     */
    void MajorCompaction(int level);

    /**
     * @brief 将合并后的SST文件从内存写回磁盘
     * @details 只会被MajorCompaction函数调用
     */
    void WriteToFile(int level, uint64_t time_stamp, uint64_t num_pair, std::map<int64_t, std::string> &new_table);

private:
    enum mode {
        normal,
        compact,
        exits
    };

    std::shared_ptr<SkipList> mem_table_;
    std::shared_ptr<SkipList> immutable_table_;

    std::string dir_;       // SSTable文件存储目录
    std::vector<int> level_num_vec_;    // 记录每一层的文件数目
    std::vector<std::set<TableCache>> sstable_meta_info_;   // 记录所有SSTable文件的元信息
    mode kvstore_mode_; // 存储引擎工作模式
    ThreadPool pool_{4};    // 线程池
    cache_t<uint64_t, std::string> cache_;  // 缓存器

    // 同步与互斥相关
    std::condition_variable cond_var_;
    std::mutex mutex_;
    std::shared_mutex rw_mutex_;
};

#endif // !LSMKVSTORE_KVSTORE_H_