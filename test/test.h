#ifndef LSMKVSTORE_TEST_H_
#define LSMKVSTORE_TEST_H_

#include <string>
#include <iostream>

#include "kvstore.h"

class Test {
public:
    Test(const std::string &dir, bool v = true) : kvstore(dir), verbose(v) {
        cnt_tests = 0;
        cnt_tests_passed = 0;
        cnt_phases = 0;
        cnt_phases_passed = 0;
    }

    virtual void start_test(void *args = NULL) = 0;

protected:
    uint64_t cnt_tests;
    uint64_t cnt_tests_passed;
    uint64_t cnt_phases;
    uint64_t cnt_phases_passed;

    class KVStore kvstore;
    bool verbose;       // 是否显示日志

    /**
     * @brief 对 expect 的封装
    */
    #define EXPECT(exp, got) expect<decltype(got)>((exp), (got), (__FILE__), (__LINE__)) // __FILE__:表示当前源文件名，类型为字符串常量，__LINE__：代表当前程序行的行号，类型为十进制整数常量

    /**
     * @brief 判断结果是否符合预期
     * @param[in] exp 预期结果
     * @param[in] got 实际结果
     * @param[in] file 文件名
     * @param[in] line 行号
     */
    template <typename T>
    void expect(const T &exp, const T &got, const std::string &file, int line) {
        ++cnt_tests;
        if (exp == got) {
            ++cnt_tests_passed;
            return;
        }
        // std::cout << "*************************" << file << std::endl;
        // for (int i = 0; i < exp.size(); ++i) {
        //     std::cout << exp[i] << ",";
        // }
        // std::cout << std::endl;
        // for (int i = 0; i < got.size(); ++i) {
        //     std::cout << got[i] << ",";
        // }
        // std::cout << std::endl;
        if (verbose) {
            // std::cerr << "Test error @" << file << ":" << line << ", expected [" << exp << "], got [" << got << "]" << std::endl;
            std::cout << "Test error @" << file << ":" << line << ", expected " << exp.size() << ", got " << got.size() << std::endl;
        }
    }

    /**
     * @brief 输出一个阶段内通过的测试数量及总的测试数量，如果相等则该阶段通过
    */
    void phase_report(void) {
        // 报告一个阶段内通过的测试数量及总的测试数量
        std::cout << "Phase " << (++cnt_phases) << " : " << cnt_tests_passed << " / " << cnt_tests;

        // 判断该阶段是否通过
        if (cnt_tests_passed == cnt_tests) {
            ++cnt_phases_passed;
            std::cout << " [PASS]" << std::endl;
        } else {
            std::cout << " [FAIL]" << std::endl;
        }

        // 重置
        cnt_tests = 0;
        cnt_tests_passed = 0;
    }

    /**
     * @brief 输出多个阶段的最终统计结果
    */
    void final_report(void) {
        std::cout << "--------------------------------------" << std::endl;
        std::cout << "Test result: " << cnt_phases_passed << " / " << cnt_phases << " passed." << std::endl;

        cnt_phases = 0;
        cnt_phases_passed = 0;
    }
};

#endif // !LSMKVSTORE_TEST_H_