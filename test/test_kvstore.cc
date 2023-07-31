#include <unistd.h>

#include "test.h"

class TestKVStore : public Test {
public:
    TestKVStore(const std::string &dir, bool v) : Test(dir, v) {}

    void start_test(void *args = NULL) override {
        std::cout << "[Simple Test]" << std::endl;
        regular_test(*(uint64_t *)args);
    }

private:
    void regular_test(uint64_t num) {
        for (uint64_t i = 0; i < num; ++i) {
            kvstore.Put(i, std::string(i + 1, 's'));
            EXPECT(std::string(i + 1, 's'), kvstore.Get(i));
        }
        phase_report();

        std::vector<std::future<std::string>> tasks_;
        for (int i = 5; i <= 100; ++i) {
            kvstore.PutTask(3 * i, std::string(3 * i + 1, 's'));
            // usleep(100);     // 延时之后就可以全部测试通过
            tasks_.emplace_back(kvstore.GetTask(3 * i));
        }
        for (int i = 5; i <= 100; ++i) {
            EXPECT(std::string(3 * i + 1, 's'), tasks_[i - 5].get());
        }
        phase_report();

        for (uint64_t i = 0; i < num; i += 2) {
            kvstore.Del(i);
        }
        for (uint64_t i = 0; i < num; ++i) {
            EXPECT((i & 1) ? std::string(i + 1, 's') : "", kvstore.Get(i));
        }
        phase_report();

        final_report();
    }
};

// ./data true
int main(int argc, char *argv[]) {
    bool verbose = true;
    uint64_t num = std::stoi(argv[2]);

    TestKVStore test(argv[1], verbose);

    test.start_test((void *)&num);

    return 0;
}