#ifndef LSMKVSTORE_THREADPOOL_H_
#define LSMKVSTORE_THREADPOOL_H_

#include <vector>
#include <thread>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <future>
#include <memory>
#include <functional>
#include <utility>

// 多个线程共享一个任务队列，一个线程负责产生任务，并将任务放到任务队列中，
// 还要在这个任务执行后获取它的返回值，多个子线程从任务队列中取出任务并执行

class ThreadPool {
public:
    explicit ThreadPool(std::size_t);
    ~ThreadPool();

    template <class F, class... Args> // 参照std::thread的初始化构造函数https://www.runoob.com/w3cnote/cpp-std-thread.html
    auto Enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    std::vector<std::thread> workers_;                       // 一组工作线程
    std::queue<std::function<void()>> tasks_;      // 任务队列
    std::atomic<bool> stop_;                                        // 停止标志
    std::mutex mutex_;                                                    // 互斥锁
    std::condition_variable cond_;                             // 条件变量
};

/**
 * @brief 线程池的构造函数
 * @param[in] thread_num 线程数量
*/
ThreadPool::ThreadPool(std::size_t thread_num) : stop_(false) {
    for (std::size_t i = 0; i < thread_num; ++i) {
        workers_.emplace_back([this]{
            while (!stop_) {
                std::function<void()> task;  // 用于存储待执行的任务
                {
                    std::unique_lock<std::mutex> lock(this->mutex_); // 创建一个独占锁对象lock，并锁定ThreadPool对象的mutex_成员
                    this->cond_.wait(lock, [this]{ return this->stop_.load() || !this->tasks_.empty(); });  // 等待条件满足
                    if (this->stop_.load() && this->tasks_.empty()) return;
                    task = std::move(this->tasks_.front()); // 从任务队列中获取一个任务，并将其移动到task对象中
                    this->tasks_.pop();
                }
                task();     // 执行所获取的任务
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    this->stop_.store(true);            // 用非原子参数替换原子对象的值
    this->cond_.notify_all();          // 唤醒所有等待的线程
    for (std::thread &worker : workers_) {
        worker.join();
    }
}

// https://zhuanlan.zhihu.com/p/147913459
template <class F, class... Args>
auto ThreadPool::Enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;
    auto task = std::make_shared<std::packaged_task<return_type()>>(
                            std::bind(std::forward<F>(f), std::forward<Args>(args)...));  // std::make_shared 可以返回一个指定类型的 std::shared_ptr

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (stop_.load()) {
            throw std::runtime_error("KVStore has been closed, cannot add task");
        }
        tasks_.emplace([task](){ (*task)(); });
    }
    cond_.notify_one();

    return res;
}

#endif // !LSMKVSTORE_THREADPOOL_H_