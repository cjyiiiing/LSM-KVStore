#ifndef LSMKVSTORE_OPTIONS_H_
#define LSMKVSTORE_OPTIONS_H_

#include <math.h>

namespace options {

// 删除标记
const std::string kDelSign = "~DELETED~";

// SST文件中的时间戳、元素个数、min_key、max_key、布隆过滤器加起来的总字节数
const int kInitialSize = 10272;

// 每层的SST文件数量上限
inline int SSTMaxNumForLevel(int i) {
    return pow(2, i + 1);
}

// SST文件的大小上限
const int kMemTable = (int)pow(2, 21);

// 缓存策略（FIFO、LRU、LFU三选一）
// #define FIFO
#define LRU
// #define LFU

// 缓存容量，可以缓存多少对键值对
const int kCacheCap = 100;

}       // namespace options

#endif // !LSMKVSTORE_OPTIONS_H_