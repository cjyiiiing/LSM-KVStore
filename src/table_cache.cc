#include "table_cache.h"

TableCache::TableCache(const std::string &file_name) {
    sst_path_ = file_name;
    Open();
}

std::string TableCache::GetValue(int64_t key) const {
    // 判断是否在min_key~max_key之间
    if (key < min_max_key_[0] || key > min_max_key_[1]) {
        return "";
    }

    // 利用布隆过滤器判断key是否存在，如果有一位为0则表示肯定不存在，如果都为1则表示可能存在
    unsigned int hash[4] = {0};
    MurmurHash3_x64_128(&key, sizeof(key), 1, hash);
    for (auto i : hash) {
        if (bloom_filter_[i % 81920] == 0) {
            return "";
        }
    }

    // 在索引区查找，如果存在该key则需要先获取value的长度再读取value
    auto iter1 = key_offset_map_.find(key);
    if (iter1 == key_offset_map_.end()) return "";
    auto iter2 = iter1;
    ++iter2;
    std::fstream file(sst_path_, std::ios::in | std::ios::binary);
    file.seekg(0, std::ios::end);   // 移动到文件末尾
    uint64_t len = (iter2 != key_offset_map_.end())
                                 ? (iter2->second - iter1->second)  // 如果key不是最后一个，则两个偏移量相减
                                 : ((int)file.tellg() - iter1->second);   // 如果key是最后一个，则文件末尾位置减偏移量
    file.seekg(iter1->second);      // 移动到key对应的value所在位置
    std::string value(len - 1, ' ');
    file.read(&(*value.begin()), sizeof(char) * len);

    file.close();

    return value;
}

void TableCache::Open() {
    std::fstream file(sst_path_, std::ios::in | std::ios::binary);

    if (file.is_open()) {
        file.read((char *)&time_and_size_, 2 * sizeof(uint64_t));
        file.read((char *)&min_max_key_, 2 * sizeof(int64_t));
        file.read((char *)&bloom_filter_, sizeof(bloom_filter_));

        uint64_t size = time_and_size_[1];
        int64_t temp_key;
        uint32_t temp_offset;
        while (size--) {
            file.read((char *)&temp_key, sizeof(int64_t));
            file.read((char *)&temp_offset, sizeof(uint32_t));
            key_offset_map_[temp_key] = temp_offset;
        }

        file.close();
    }
}

void TableCache::Traverse(std::map<int64_t, std::string> &pair) const {
    std::fstream file(sst_path_, std::ios::in | std::ios::binary);
    auto iter1 = key_offset_map_.begin();
    auto iter2 = iter1;
    iter2++;
    uint64_t len = 0;

    // 循环读取key对应的value
    while (iter1 != key_offset_map_.end()) {
        // 获取value的长度
        if (iter2 != key_offset_map_.end()) {
            len = iter2->second - iter1->second;
            iter2++;
        } else {
            file.seekg(0, std::ios::end);
            len = (int)file.tellg() - iter1->second;
        }

        // 读取value
        file.seekg(iter1->second);
        std::string value(len, ' ');
        file.read(&(*value.begin()), sizeof(char) * len);
        pair[iter1->first] = value;
        iter1++;
    }

    file.close();
}
