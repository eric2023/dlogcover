/**
 * @file work_stealing_thread_pool.h
 * @brief 工作窃取线程池实现 - 提供更高效的并行文件分析
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_UTILS_WORK_STEALING_THREAD_POOL_H
#define DLOGCOVER_UTILS_WORK_STEALING_THREAD_POOL_H

#include <vector>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <random>

namespace dlogcover {
namespace utils {

/**
 * @brief 工作窃取队列
 * 
 * 每个线程拥有自己的任务队列，支持从其他线程窃取任务
 */
class WorkStealingQueue {
public:
    WorkStealingQueue() = default;
    ~WorkStealingQueue() = default;

    /**
     * @brief 向队列末尾添加任务（本线程专用）
     * @param task 任务函数
     */
    void pushBack(std::function<void()> task);

    /**
     * @brief 从队列末尾取出任务（本线程专用）
     * @return 任务函数，如果队列为空则返回nullptr
     */
    std::function<void()> popBack();

    /**
     * @brief 从队列前端窃取任务（其他线程使用）
     * @return 任务函数，如果队列为空则返回nullptr
     */
    std::function<void()> steal();

    /**
     * @brief 检查队列是否为空
     * @return 如果为空返回true
     */
    bool empty() const;

    /**
     * @brief 获取队列大小
     * @return 队列中的任务数量
     */
    size_t size() const;

private:
    mutable std::mutex mutex_;
    std::deque<std::function<void()>> queue_;
};

/**
 * @brief 工作窃取线程池
 * 
 * 提供高效的并行任务执行，每个线程维护自己的任务队列，
 * 空闲线程可以从其他线程窃取任务，减少锁竞争和负载不均衡
 */
class WorkStealingThreadPool {
public:
    /**
     * @brief 构造函数
     * @param numThreads 线程数量，0表示使用硬件并发数
     */
    explicit WorkStealingThreadPool(size_t numThreads = 0);

    /**
     * @brief 析构函数
     */
    ~WorkStealingThreadPool();

    /**
     * @brief 提交任务到线程池
     * @tparam F 函数类型
     * @tparam Args 参数类型
     * @param f 要执行的函数
     * @param args 函数参数
     * @return 任务的future对象
     */
    template<class F, class... Args>
    auto submit(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;

    /**
     * @brief 批量提交任务
     * @param tasks 任务列表
     * @return future对象列表
     */
    std::vector<std::future<void>> submitBatch(std::vector<std::function<void()>>&& tasks);

    /**
     * @brief 获取线程数量
     * @return 线程数量
     */
    size_t getThreadCount() const { return workers_.size(); }

    /**
     * @brief 获取待处理任务数量
     * @return 所有队列中的任务总数
     */
    size_t getTotalQueueSize() const;

    /**
     * @brief 关闭线程池
     */
    void shutdown();

    /**
     * @brief 检查线程池是否已停止
     * @return 如果已停止返回true
     */
    bool isStopped() const { return stop_.load(); }

    /**
     * @brief 获取性能统计信息
     * @return 包含工作窃取次数、任务执行次数等统计信息的字符串
     */
    std::string getPerformanceStats() const;

private:
    /**
     * @brief 工作线程函数
     * @param threadId 线程ID
     */
    void workerThread(size_t threadId);

    /**
     * @brief 尝试获取任务
     * @param threadId 当前线程ID
     * @return 任务函数，如果没有任务则返回nullptr
     */
    std::function<void()> getTask(size_t threadId);

    /**
     * @brief 尝试从其他线程窃取任务
     * @param threadId 当前线程ID
     * @return 任务函数，如果窃取失败则返回nullptr
     */
    std::function<void()> stealTask(size_t threadId);

    // 工作线程
    std::vector<std::thread> workers_;
    
    // 每个线程的任务队列
    std::vector<std::unique_ptr<WorkStealingQueue>> queues_;
    
    // 下一个要分配任务的队列索引（轮询分配）
    std::atomic<size_t> nextQueueIndex_{0};
    
    // 同步原语
    mutable std::mutex globalMutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
    
    // 性能统计
    mutable std::atomic<size_t> totalTasksExecuted_{0};
    mutable std::atomic<size_t> totalStealsAttempted_{0};
    mutable std::atomic<size_t> totalStealsSuccessful_{0};
    
    // 随机数生成器（用于选择窃取目标）
    thread_local static std::random_device rd_;
    thread_local static std::mt19937 gen_;
};

// 模板方法实现
template<class F, class... Args>
auto WorkStealingThreadPool::submit(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<return_type> res = task->get_future();
    
    if (stop_.load()) {
        throw std::runtime_error("submit on stopped WorkStealingThreadPool");
    }

    // 轮询方式选择队列，减少热点竞争
    size_t queueIndex = nextQueueIndex_.fetch_add(1) % queues_.size();
    
    queues_[queueIndex]->pushBack([task](){ (*task)(); });
    
    // 通知工作线程
    condition_.notify_one();
    
    return res;
}

} // namespace utils
} // namespace dlogcover

#endif // DLOGCOVER_UTILS_WORK_STEALING_THREAD_POOL_H 