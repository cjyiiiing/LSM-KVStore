#ifndef LSMKVSTORE_SKIPLIST_H_ // <PROJECT>_<PATH>_<FILE>_H_
#define LSMKVSTORE_SKIPLIST_H_

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <bitset>
#include <string.h>

#include "options.h"
#include "murmurhash3.h"

struct Node {
    int64_t key_;
    std::string val_;
    Node *right_, *down_;
    Node(int64_t key, const std::string &val, Node *right, Node *down)
        : key_(key), val_(val), right_(right), down_(down) {}
    Node() : key_(INT64_MIN), val_(""), right_(nullptr), down_(nullptr) {}
};

class SkipList {
public:
    size_t memory_;     // 转换成SSTable占用的空间大小

public:
    SkipList()
        : size_(0),
          head_(nullptr),
          memory_(options::kInitialSize),
          time_stamp_(0),
          min_key_(INT64_MAX),
          max_key_(INT64_MIN) {}

    ~SkipList();

    /**
     * @brief 查找键值对
     * @param[in] key 查找键值对的键值
     * @return string 返回key对应的val，如果key不存在则返回""
     */
    std::string Get(int64_t key) const;

    /**
     * @brief 插入键值对
     * @param[in] key 键
     * @param[in] val 值
     */
    void Put(int64_t key, const std::string &val);

    /**
     * @brief 将MemTable储存为L0层SSTable, Minor MinorCompaction
     * @param[in] num
     * @param[in] dir
     */
    void Store(int num, const std::string &dir);

    /**
     * @brief 获取第一个节点
     */
    Node *GetFirstNode() const;

    /**
     * @brief 获取储存的键值对个数
     */
    size_t GetSize() const { return size_; }

private:
    uint64_t size_;                  // 储存的键值对个数
    Node *head_;                    // 头节点
    uint64_t time_stamp_; // 时间戳，即SSTable序号
    int64_t min_key_;           // 最小key
    int64_t max_key_;          // 最大key
};

#endif // !LSMKVSTORE_SKIPLIST_H_