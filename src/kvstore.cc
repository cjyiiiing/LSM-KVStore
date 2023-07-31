#include "kvstore.h"

/**
 * @brief 获取SST文件名中包含的序号(SSTable0, SSTable1,...)
 * @param[in] file_name 当前文件夹下的最后一个SST文件
 * @return int 序号
 */
inline int GetFileNum(const std::string file_name) {
    auto iter = file_name.find('e');
    iter++;
    std::string str = file_name.substr(iter);
    return std::stoi(str);
}

/**
 * @details 初始化成员变量
 * 将dir目录下的所有SST文件的元信息缓存到sstable_meta_info_中
 * 记录level_num_vec_
 */
KVStore::KVStore(const std::string &dir) : KVStoreAPI(dir), cache_(options::kCacheCap) {
    mem_table_ = std::make_shared<SkipList>();
    dir_ = dir;
    kvstore_mode_ = normal;
    level_num_vec_.emplace_back(0);  // 没有这行的话会出现段错误
    Reset();   // todo 不清空data文件夹的话测试有时会被杀死

    std::vector<std::string> dirs;
    int dir_num = utils::ScanDir(dir, dirs);
    for (int i = 0; i < dir_num; ++i) {
        sstable_meta_info_.emplace_back();
        std::string dir_path = dir + "/" + dirs[i];
        std::vector<std::string> files;
        int file_num = utils::ScanDir(dir_path, files);     // files是有序的
        // 填充level_num_vec_
        if (file_num > 0) {
            level_num_vec_.emplace_back(GetFileNum(files.back()));
        } else {
            level_num_vec_.emplace_back(0);
        }
        // 填充sstable_meta_info_
        for (int j = 0; j < file_num; ++j) {
            std::string file_name = dir_path + "/" + files[j];
            TableCache tc(file_name);
            sstable_meta_info_[i].insert(std::move(tc));
        }
    }

    if (sstable_meta_info_.empty()) {
        sstable_meta_info_.emplace_back();
    }
}

/**
 * @brief 将内存中的数据dump到L0层，若L0层SST文件数量超过限制，则触发Compaction
*/
KVStore::~KVStore() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_var_.wait(lock, [&]{ return kvstore_mode_ == normal; });
    kvstore_mode_ = exits;
    std::string path = dir_ + "/level0";
    mem_table_->Store(++level_num_vec_[0], path);
    if (sstable_meta_info_[0].size() >= options::SSTMaxNumForLevel(0)) {
        MajorCompaction(1);
    }
}

void KVStore::Put(uint64_t key, const std::string &val, bool to_cache) {
    std::unique_lock<std::shared_mutex> lock(rw_mutex_); // 只有一个线程能写

    std::string cur_val = mem_table_->Get(key);
    if (!cur_val.empty()) {     // 如果存在这个key，替换value
        mem_table_->memory_ += strlen(val.c_str()) - strlen(cur_val.c_str());
    } else {        // 如果不存在这个key，插入
        mem_table_->memory_ += strlen(val.c_str()) + 1 + 12; // '\0' + (key + offset)
    }

    // 如果mem_table_满了，转换为immutable_table_并创建线程写入磁盘
    if (mem_table_->memory_ > options::kMemTable) {
        std::unique_lock<std::mutex> lk(mutex_);
        // 如果immutable_table_非空，先等它写入到level0
        if (immutable_table_ != nullptr) {
            // std::unique_lock<std::mutex> lk(mutex_);
            cond_var_.wait(lk, [&]{ return kvstore_mode_ == normal; });
        }

        immutable_table_ = mem_table_;
        mem_table_ = std::make_shared<SkipList>();
        mem_table_->memory_ += strlen(val.c_str()) + 1 + 12;

        std::thread compact_thread(&KVStore::MinorCompaction, this);
        compact_thread.detach();
    }

    mem_table_->Put(key, val);

    if (to_cache) cache_.Put(key, val);
}

// 将Put函数封装为任务，以便丢进线程池
void KVStore::PutTask(uint64_t key, const std::string &val, bool to_cache) {
    pool_.Enqueue(&KVStore::Put, this, key, val, to_cache);
}

std::string KVStore::Get(uint64_t key) {
    std::shared_lock<std::shared_mutex> lock(rw_mutex_);    // 多个线程能同时读

    // 1、如果cache中有，则直接返回
    if (cache_.Cached(key)) return cache_.Get(key);

    // 2、查mem_table_
    std::string val = mem_table_->Get(key);
    if (!val.empty()) {
        if (val == options::kDelSign) {
            return "";
        } else {
            return val;
        }
    }

    // 3、查immutable_table_
    if (immutable_table_ != nullptr) {
        // 增加引用计数，避免被析构
        auto ptr = immutable_table_;
        val = immutable_table_->Get(key);
        if (!val.empty()) {
            if (val == options::kDelSign) {
                return "";
            } else {
                return val;
            }
        }

        ptr.reset();

        while (kvstore_mode_ == compact) {
            std::unique_lock<std::mutex> lk(mutex_);
            cond_var_.wait(lk, [&]{ return kvstore_mode_ == normal; });
        }
    }

    // 4、查SST文件   //TODO：不需要加互斥锁吗
    for (const auto &table_list : sstable_meta_info_) {
        for (const auto &table : table_list) {
            val = table.GetValue(key);
            if (!val.empty()) {
                if (val == options::kDelSign) {
                    return "";
                } else {
                    return val;
                }
            }
        }
    }

    return "";
}

// 将Get函数封装为任务，以便丢进线程池。返回一个包含key对应val的future对象
std::future<std::string> KVStore::GetTask(uint64_t key) {
    return pool_.Enqueue(&KVStore::Get, this, key);
}

bool KVStore::Del(uint64_t key, bool to_cache) {
    Put(key, options::kDelSign);
    if (to_cache) cache_.Remove(key);
    return true;
}

// 将Del函数封装为任务，以便丢进线程池
void KVStore::DelTask(uint64_t key, bool to_cache) {
    pool_.Enqueue(&KVStore::Del, this, key, to_cache);
}

void KVStore::Reset() {
    std::vector<std::string> dirs;
    int dir_num = utils::ScanDir(dir_, dirs);
    for (int i = 0; i < dir_num; ++i) {
        std::string dir_path = dir_ + "/" + dirs[i];
        std::vector<std::string> files;
        int file_num = utils::ScanDir(dir_path, files);
        for (int j = 0; j < file_num; ++j) {
            utils::RmFile((dir_path + "/" + files[j]).c_str());
        }
        utils::RmDir(dir_path.c_str());
    }
}

void KVStore::MinorCompaction() {
    std::unique_lock<std::mutex> lock(mutex_);

    kvstore_mode_ = compact;

    // 保存到level0层
    std::string path = dir_ + "/level0";
    if (!utils::DirExists(path)) utils::MkDir(path.c_str());
    immutable_table_->Store(++level_num_vec_[0], path);

    // 修改sstable_meta_info_
    std::string file_name = path + "/SSTable" + std::to_string(level_num_vec_[0]) + ".sst";
    TableCache tc(file_name);
    sstable_meta_info_[0].insert(std::move(tc));

    // 检查level0层是否需要compaction
    MajorCompaction(1);

    immutable_table_ = nullptr;
    // std::cout << "immutable_table_ = nullptr;" << std::endl;
    kvstore_mode_ = normal;
    lock.unlock();
    cond_var_.notify_one();
}

/**
 * @brief 更新minkey_sstindex
 * @param[in] kv_to_compact 被合并的键值对
 * @param[in] kv_to_compact_iter 被合并的键值对的迭代器
 * @param[in] minkey_sstindex 各个SST文件中的最小键及其所在的SST文件的索引
 * @param[in] index SST文件序号
 */
static void Update(std::vector<std::map<int64_t, std::string>> &kv_to_compact,
                                       std::vector<std::map<int64_t, std::string>::iterator> &kv_to_compact_iter,
                                       std::map<int64_t, int> &minkey_sstindex, int index) {
    while (kv_to_compact_iter[index] != kv_to_compact[index].end()) {
        int64_t key = kv_to_compact_iter[index]->first;
        if (minkey_sstindex.count(key) == 0) {
            minkey_sstindex[key] = index;
            break;
        } else if (index > minkey_sstindex[key]) {
            int temp_index = minkey_sstindex[key];
            minkey_sstindex[key] = index;
            Update(kv_to_compact, kv_to_compact_iter, minkey_sstindex, temp_index);
            break;
        }
        kv_to_compact_iter[index]++;
    }
}

void KVStore::MajorCompaction(int level) {
    // 如果level-1层的SST文件数量小于上限，则不需要合并
    int sst_num_for_levelminus1 = sstable_meta_info_[level - 1].size();
    if (sst_num_for_levelminus1 <= options::SSTMaxNumForLevel(level - 1)) {
        return;
    }

    // 判断当前层目录是否存在，不存在则创建目录
    std::string path_level = dir_ + "/level" + std::to_string(level);
    if (!utils::DirExists(path_level)) {
        utils::MkDir(path_level.c_str());
        level_num_vec_.emplace_back(0);
        sstable_meta_info_.emplace_back();
    }

    // 判断是否到目前的最后一层，后续用于滤除最后一层中有删除标记的数据
    bool last_level = false;
    if (level == sstable_meta_info_.size() - 1) last_level = true;

    // 记录level-1层需要被删除的文件
    std::vector<TableCache> file_to_rm_levelminus1;
    // 记录level层需要被删除的文件
    std::vector<TableCache> file_to_rm_level;
    // 记录需要合并的所有文件  小顶堆
    std::priority_queue<TableCache, std::vector<TableCache>, std::greater<>> sort_table_to_merge;
    // 需要合并的文件数
    int compact_num = (level - 1 == 0) ?
                                               sst_num_for_levelminus1 :
                                               (sst_num_for_levelminus1 - options::SSTMaxNumForLevel(level - 1));

    // 遍历level - 1层中将被合并的SST文件，获取时间戳和最小最大key
    uint64_t time_stamp = 0;
    int64_t temp_min = INT64_MAX, temp_max = INT64_MIN;
    auto iter = sstable_meta_info_[level - 1].begin();
    for (int i = 0; i < compact_num; ++i) {
        TableCache table = *iter;
        iter++;

        sort_table_to_merge.emplace(table);
        file_to_rm_levelminus1.emplace_back(table);

        time_stamp = table.GetTimeStamp();      // 时间戳应该用最小的还是最大的？
        if (table.GetMinKey() < temp_min) temp_min = table.GetMinKey();
        if (table.GetMaxKey() > temp_max) temp_max = table.GetMaxKey();
    }

    // 找到level层与level-1层的key有交集的文件
    for (auto &table : sstable_meta_info_[level]) {
        if (table.GetMinKey() <= temp_max && table.GetMaxKey() >= temp_min) {
            sort_table_to_merge.emplace(table);
            file_to_rm_level.emplace_back(table);
        }
    }

    // 被合并的键值对，下标越大时间戳越大
    std::vector<std::map<int64_t, std::string>> kv_to_compact;
    // 被合并的键值对的迭代器
    std::vector<std::map<int64_t, std::string>::iterator> kv_to_compact_iter;
    // 各个SST文件中的最小键及其所在的SST文件的索引
    std::map<int64_t, int> minkey_sstindex;

    // 从小顶堆中依次取出堆顶TableCache，并将其中的键值对全部读进内存，插入到vector中
    while (!sort_table_to_merge.empty()) {
        std::map<int64_t, std::string> kvpair;
        sort_table_to_merge.top().Traverse(kvpair);
        sort_table_to_merge.pop();
        kv_to_compact.emplace_back(kvpair);     // 按照时间戳的顺序插入
    }

    // 根据kv_to_compact的大小相应地调整kv_to_compact_iter的大小
    kv_to_compact_iter.resize(kv_to_compact.size());

    // 获取每个SST文件中的最小key，如果key相同则保留时间戳最大的（通过倒序遍历来实现）
    for (int i = kv_to_compact.size() - 1; i >=0; --i) {
        auto iter = kv_to_compact[i].begin();
        while (iter != kv_to_compact[i].end()) {
            if (minkey_sstindex.count(iter->first) == 0) {
                minkey_sstindex[iter->first] = i;
                break;
            }
            iter++;
        }
        if (iter != kv_to_compact[i].end()) {
            kv_to_compact_iter[i] = iter;
        }
    }

    // SST文件大小（初始值为除了索引区和数据区之外的固定大小）
    int size = options::kInitialSize;
    // 暂存合并后的键值对
    std::map<int64_t, std::string> new_table;
    // 当前最小key及其对应的value和SST文件序号
    int64_t temp_key;
    std::string temp_value;
    int index;

    // 依次读取当前最小键值对，保存到new_table，若数据大小达到上限则写入文件
    while (!minkey_sstindex.empty()) {
        auto iter = minkey_sstindex.begin();
        temp_key = iter->first;
        index = iter->second;
        temp_value = kv_to_compact[index][temp_key];
        // temp_value = kv_to_compact_iter[index]->second;
        if (!last_level || temp_value != options::kDelSign) {   // 有删除标记的不写入文件
            size += strlen(temp_value.c_str()) + 1 + 12;           // 1: '\0', 12: key + offset的大小
            if (size > options::kMemTable) {
                WriteToFile(level, time_stamp, new_table.size(), new_table);
                size = options::kInitialSize + strlen(temp_value.c_str()) + 1 + 12;
            }
            new_table[temp_key] = temp_value;
        }
        minkey_sstindex.erase(temp_key);
        kv_to_compact_iter[index]++;
        Update(kv_to_compact, kv_to_compact_iter, minkey_sstindex, index);
    }

    // 剩下的数据也写入文件
    if (!new_table.empty()) {
        WriteToFile(level, time_stamp, new_table.size(), new_table);
    }

    // 删除level-1和level层被合并的文件
    for (auto &table : file_to_rm_levelminus1) {   // 没有修改level_num_vec_
        utils::RmFile(table.GetFileName().c_str());
        sstable_meta_info_[level - 1].erase(table);
    }
    for (auto &table : file_to_rm_level) {
        utils::RmFile(table.GetFileName().c_str());
        sstable_meta_info_[level].erase(table);
    }

    // 递归地判断下一层
    MajorCompaction(level + 1);
}

void KVStore::WriteToFile(int level, uint64_t time_stamp, uint64_t num_pair,
                                                      std::map<int64_t, std::string> &new_table) {
    std::string path = dir_ + "/level" + std::to_string(level);
    level_num_vec_[level]++;
    std::string file_name = path + "/SSTable" + std::to_string(level_num_vec_[level]) + ".sst";
    std::fstream out_file(file_name, std::ios::app | std::ios::binary);

    auto iter1 = new_table.begin();
    int64_t min_key = iter1->first;
    auto iter2 = new_table.rbegin();
    int64_t max_key = iter2->first;

    // 写入时间戳、键值对个数、最小键、最大键
    out_file.write((char *)(&time_stamp), sizeof(uint64_t));
    out_file.write((char *)(&num_pair), sizeof(uint64_t));
    out_file.write((char *)(&min_key), sizeof(int64_t));
    out_file.write((char *)(&max_key), sizeof(int64_t));

    // 写入布隆过滤器
    std::bitset<81920> filter;
    int64_t temp_key;
    const char *temp_value;
    unsigned int hash[4] = {0};
    while (iter1 != new_table.end()) {
        temp_key = iter1->first;
        MurmurHash3_x64_128(&temp_key, sizeof(temp_key), 1, hash);
        for (auto i : hash) {
            filter.set(i % 81920);
        }
        iter1++;
    }
    out_file.write((char *)(&filter), sizeof(filter));

    // 写入索引区
    const uint32_t val_start_area = 10272 + num_pair * 12; // 4 * 8  + 81920 / 8 + 索引区的长度
    uint32_t index = 0;
    iter1 = new_table.begin();
    int offset = 0;
    while (iter1 != new_table.end()) {
        temp_key = iter1->first;
        index = val_start_area + offset;
        out_file.write((char *)(&temp_key), sizeof(int64_t));
        out_file.write((char *)(&index), sizeof(uint32_t));
        offset += strlen((iter1->second).c_str()) + 1;
        iter1++;
    }

    // 写入数据区
    iter1 = new_table.begin();
    while (iter1 != new_table.end()) {
        temp_value = (iter1->second).c_str();
        out_file.write(temp_value, sizeof(char) * strlen((iter1->second).c_str()));
        temp_value = "\0";
        out_file.write(temp_value, sizeof(char) * 1);
        iter1++;
    }

    out_file.close();

    TableCache tc(file_name);
    sstable_meta_info_[level].insert(std::move(tc));

    new_table.clear();
}

