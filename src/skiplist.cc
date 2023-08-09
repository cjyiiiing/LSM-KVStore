#include "skiplist.h"

SkipList::~SkipList() {
    // std::cout << "SkipList::~SkipList()" << std::endl;

    // 释放Node数据
    Node *p = head_, *q = nullptr, *down = nullptr;
    while (p != nullptr) {
        down = p->down_;
        while (p != nullptr) {
            q = p->right_;
            delete p;
            p = q;
        }
        p = down;
    }

    // 重置成员变量
    memory_ = options::kInitialSize; // 时间戳+元素个数+最小最大key+布隆过滤器所占的字节数
    head_ = nullptr;
    size_ = 0;
    min_key_ = INT64_MAX;
    max_key_ = INT64_MIN;
}

std::string SkipList::Get(int64_t key) const {
    if (size_ == 0) return "";

    Node *p = head_;
    while (p != nullptr) {
        while (p->right_ != nullptr && p->right_->key_ < key) {
            p = p->right_;
        }
        if (p->right_ != nullptr && p->right_->key_ == key) {
            return (p->right_->val_);
        }
        p = p->down_;
    }

    return ""; // 找不到返回""
}

void SkipList::Put(int64_t key, const std::string &val) {
    // 更新最大最小key
    if (key < min_key_)
        min_key_ = key;
    if (key > max_key_)
        max_key_ = key;

    std::vector<Node *> pre_node_list; // 记录每一层的前一节点
    Node *p = head_;
    while (p != nullptr) {
        while (p->right_ != nullptr && p->right_->key_ <= key) {
            p = p->right_;
        }
        pre_node_list.emplace_back(p);
        p = p->down_;
    }

    // 存在相同的key：替换
    if (!pre_node_list.empty() && pre_node_list.back()->key_ == key) {
        while (!pre_node_list.empty() && pre_node_list.back()->key_ == key) {
            Node *node = pre_node_list.back();
            pre_node_list.pop_back();
            node->val_ = val;
        }
        return;
    }

    // 不存在相同的key：插入
    bool insert_up_flag = true;
    Node *down_node = nullptr;
    ++size_;
    while (insert_up_flag && !pre_node_list.empty()) {
        Node *insert_node = pre_node_list.back();
        pre_node_list.pop_back();
        insert_node->right_ = new Node(key, val, insert_node->right_, down_node);
        down_node = insert_node->right_;
        insert_up_flag = (rand() & 1); // 向上增加层数的概率为50%
    }
    if (insert_up_flag) { // 如果pre_node_list空了但还要向上增加层数
        Node *old_head = head_;
        head_ = new Node();
        head_->right_ = new Node(key, val, nullptr, down_node);
        head_->down_ = old_head;
    }
}

// 写入时间戳、键值对个数、最小键、最大键、布隆过滤器、索引区、数据区
void SkipList::Store(int num, const std::string &dir) {
    std::string file_name = dir + "/SSTable" + std::to_string(num) + ".sst";
    std::fstream out_file(file_name, std::ios::app | std::ios::binary);

    Node *node = GetFirstNode()->right_;

    ++time_stamp_; // TODO

    // 写入时间戳、键值对个数、最小键、最大键
    out_file.write((char *)(&time_stamp_), sizeof(uint64_t));
    out_file.write((char *)(&size_), sizeof(uint64_t));
    out_file.write((char *)(&min_key_), sizeof(int64_t));
    out_file.write((char *)(&max_key_), sizeof(int64_t));

    // 写入布隆过滤器
    std::bitset<81920> filter;
    int64_t temp_key;
    const char *temp_value;
    unsigned int hash[4] = {0};
    while (node != nullptr) {
        temp_key = node->key_;
        MurmurHash3_x64_128(&temp_key, sizeof(temp_key), 1, hash);
        for (auto i : hash) {
            filter.set(i % 81920);
        }
        node = node->right_;
    }
    out_file.write((char *)(&filter), sizeof(filter));

    // 写入索引区
    const uint32_t val_start_area = options::kInitialSize + size_ * 12; // 4 * 8  + 81920 / 8 + 索引区的长度
    uint32_t index = 0;
    node = GetFirstNode()->right_;
    int offset = 0;
    while (node != nullptr) {
        temp_key = node->key_;
        index = val_start_area + offset;
        out_file.write((char *)(&temp_key), sizeof(int64_t));
        out_file.write((char *)(&index), sizeof(uint32_t));
        offset += strlen((node->val_).c_str()) + 1;
        node = node->right_;
    }

    // 写入数据区
    node = GetFirstNode()->right_;
    while (node != nullptr) {
        temp_value = (node->val_).c_str();
        out_file.write(temp_value, sizeof(char) * strlen(node->val_.c_str()));
        temp_value = "\0";
        out_file.write(temp_value, sizeof(char) * 1);
        node = node->right_;
    }

    out_file.close();
}

Node *SkipList::GetFirstNode() const {
    Node *p = head_;
    while (p->down_ != nullptr) {
        p = p->down_;
    }
    return p;
}