#ifndef LSMKVSTORE_TABLE_CACHE_H_
#define LSMKVSTORE_TABLE_CACHE_H_

#include <string>
#include <cstddef>
#include <bitset>
#include <map>
#include <fstream>

#include "murmurhash3.h"

/**
 * @brief SST文件类
*/
class TableCache {
public:
    TableCache() { sst_path_ = ""; }
    TableCache(const std::string &file_name);

    /**
     * @brief TableCache类的小于运算符重载
     * @details: 时间戳小的排前面，时间戳相等的min_key小的排前面
    */
    bool operator<(const TableCache &temp) const {
        return (GetTimeStamp() == temp.GetTimeStamp()) ?
                      (GetMinKey() < temp.GetMinKey()) : (GetTimeStamp() < temp.GetTimeStamp());
    }

    /**
     * @brief TableCache类的大于运算符重载
     * @details: 时间戳大的排前面，时间戳相等的min_key大的排前面
    */
    bool operator>(const TableCache &temp) const {
        return (GetTimeStamp() == temp.GetTimeStamp()) ?
                      (GetMinKey() > temp.GetMinKey()) : (GetTimeStamp() > temp.GetTimeStamp());
    }

    /**
     * @brief 获取SST文件中指定key对应的value
     * @param[in] key 键
     * @return 如果key存在返回对应value，如果key不存在返回""
    */
   std::string GetValue(int64_t key) const;

   /**
    * @brief 打开SST文件，读入该文件中的各项元信息
   */
    void Open();

    /**
     * @brief 将该SST文件的键值对全部读进内存
     * @param[out] pair 读进内存的键值对的存放位置
    */
    void Traverse(std::map<int64_t, std::string> &pair) const;

    // 获取成员属性的接口
    std::string GetFileName() const { return sst_path_; }
    uint64_t GetTimeStamp() const { return time_and_size_[0]; }
    uint64_t GetPairNum() const { return time_and_size_[1]; }
    int64_t GetMinKey() const { return min_max_key_[0]; }
    int64_t GetMaxKey() const { return min_max_key_[1]; }

private:
    std::string sst_path_;                              // SST文件的路径及文件名
    uint64_t time_and_size_[2];                    // 时间戳、元素个数
    int64_t min_max_key_[2];                        // 最小最大key
    std::bitset<81920> bloom_filter_;    // 布隆过滤器
    std::map<int64_t, uint32_t> key_offset_map_;    // key及其对应的偏移量
};

#endif // !LSMKVSTORE_TABLE_CACHE_H_