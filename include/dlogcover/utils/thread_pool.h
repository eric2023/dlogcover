/**
 * @file thread_pool.h
 * @brief 线程池实现 - 用于并行文件分析
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_UTILS_THREAD_POOL_H
#define DLOGCOVER_UTILS_THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>

namespace dlogcover {
namespace utils {

/**
 * @brief 线程池类
 * 
 * 提供固定大小的线程池，用于并行执行任务
 */
class ThreadPool {
public:
    /**
     * @brief 构造函数
     * @param numThreads 线程数量，0表示使用硬件并发数
     */
    explicit ThreadPool(size_t numThreads = 0);

    /**
     * @brief 析构函数
     */
    ~ThreadPool();

    /**
     * @brief 提交任务到线程池
     * @tparam F 函数类型
     * @tparam Args 参数类型
     * @param f 要执行的函数
     * @param args 函数参数
     * @return 任务的future对象
     */
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;

    /**
     * @brief 获取线程数量
     * @return 线程数量
     */
    size_t getThreadCount() const { return workers_.size(); }

    /**
     * @brief 获取待处理任务数量
     * @return 任务队列大小
     */
    size_t getQueueSize() const;

    /**
     * @brief 关闭线程池
     */
    void shutdown();

    /**
     * @brief 检查线程池是否已停止
     * @return 如果已停止返回true
     */
    bool isStopped() const { return stop_.load(); }

private:
    // 工作线程
    std::vector<std::thread> workers_;
    
    // 任务队列
    std::queue<std::function<void()>> tasks_;
    
    // 同步原语
    mutable std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
};

// 模板方法实现
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queueMutex_);

        // 不允许在停止的线程池中添加任务
        if (stop_.load()) {
            throw std::runtime_error("enqueue on stopped ThreadPool");
        }

        tasks_.emplace([task](){ (*task)(); });
    }
    
    condition_.notify_one();
    return res;
}

} // namespace utils
} // namespace dlogcover

#endif // DLOGCOVER_UTILS_THREAD_POOL_H 