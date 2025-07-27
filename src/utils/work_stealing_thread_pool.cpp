/**
 * @file work_stealing_thread_pool.cpp
 * @brief 工作窃取线程池实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/utils/work_stealing_thread_pool.h>
#include <dlogcover/utils/log_utils.h>
#include <algorithm>
#include <sstream>

namespace dlogcover {
namespace utils {

// Thread-local 静态成员初始化
thread_local std::random_device WorkStealingThreadPool::rd_;
thread_local std::mt19937 WorkStealingThreadPool::gen_(WorkStealingThreadPool::rd_());

// WorkStealingQueue 实现

void WorkStealingQueue::pushBack(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(std::move(task));
}

std::function<void()> WorkStealingQueue::popBack() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return nullptr;
    }
    
    auto task = std::move(queue_.back());
    queue_.pop_back();
    return task;
}

std::function<void()> WorkStealingQueue::steal() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return nullptr;
    }
    
    auto task = std::move(queue_.front());
    queue_.pop_front();
    return task;
}

bool WorkStealingQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t WorkStealingQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

// WorkStealingThreadPool 实现

WorkStealingThreadPool::WorkStealingThreadPool(size_t numThreads) {
    // 确定线程数
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) {
            numThreads = 4; // 默认4个线程
        }
    }
    
    // 限制最大线程数
    numThreads = std::min(numThreads, static_cast<size_t>(64));
    
    LOG_INFO_FMT("初始化工作窃取线程池，线程数: %zu", numThreads);
    
    // 创建任务队列
    queues_.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        queues_.emplace_back(std::make_unique<WorkStealingQueue>());
    }
    
    // 创建工作线程
    workers_.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this, i] {
            workerThread(i);
        });
    }
}

WorkStealingThreadPool::~WorkStealingThreadPool() {
    shutdown();
}

void WorkStealingThreadPool::shutdown() {
    if (stop_.load()) {
        return; // 已经停止
    }
    
    LOG_INFO("关闭工作窃取线程池");
    
    {
        std::unique_lock<std::mutex> lock(globalMutex_);
        stop_.store(true);
    }
    
    // 通知所有线程
    condition_.notify_all();
    
    // 等待所有线程完成
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
    queues_.clear();
    
    LOG_INFO_FMT("线程池性能统计 - 总执行任务: %zu, 窃取尝试: %zu, 窃取成功: %zu, 成功率: %.1f%%",
                totalTasksExecuted_.load(),
                totalStealsAttempted_.load(), 
                totalStealsSuccessful_.load(),
                totalStealsAttempted_.load() > 0 ? 
                    (totalStealsSuccessful_.load() * 100.0 / totalStealsAttempted_.load()) : 0.0);
}

std::vector<std::future<void>> WorkStealingThreadPool::submitBatch(std::vector<std::function<void()>>&& tasks) {
    std::vector<std::future<void>> futures;
    futures.reserve(tasks.size());
    
    if (stop_.load()) {
        throw std::runtime_error("submitBatch on stopped WorkStealingThreadPool");
    }
    
    // 批量提交任务，使用轮询分配减少竞争
    size_t startQueue = nextQueueIndex_.load();
    for (size_t i = 0; i < tasks.size(); ++i) {
        auto task = std::make_shared<std::packaged_task<void()>>(std::move(tasks[i]));
        futures.emplace_back(task->get_future());
        
        size_t queueIndex = (startQueue + i) % queues_.size();
        queues_[queueIndex]->pushBack([task](){ (*task)(); });
    }
    
    nextQueueIndex_.store(startQueue + tasks.size());
    
    // 通知所有工作线程
    condition_.notify_all();
    
    return futures;
}

size_t WorkStealingThreadPool::getTotalQueueSize() const {
    size_t total = 0;
    for (const auto& queue : queues_) {
        total += queue->size();
    }
    return total;
}

std::string WorkStealingThreadPool::getPerformanceStats() const {
    std::ostringstream stats;
    stats << "工作窃取线程池性能统计:\n";
    stats << "  线程数: " << workers_.size() << "\n";
    stats << "  总执行任务: " << totalTasksExecuted_.load() << "\n";
    stats << "  窃取尝试: " << totalStealsAttempted_.load() << "\n";
    stats << "  窃取成功: " << totalStealsSuccessful_.load() << "\n";
    stats << "  窃取成功率: ";
    
    if (totalStealsAttempted_.load() > 0) {
        stats << (totalStealsSuccessful_.load() * 100.0 / totalStealsAttempted_.load()) << "%\n";
    } else {
        stats << "N/A\n";
    }
    
    stats << "  当前队列总任务数: " << getTotalQueueSize();
    
    return stats.str();
}

void WorkStealingThreadPool::workerThread(size_t threadId) {
    LOG_DEBUG_FMT("工作窃取线程 %zu 启动", threadId);
    
    while (!stop_.load()) {
        std::function<void()> task = getTask(threadId);
        
        if (task) {
            try {
                task();
                totalTasksExecuted_.fetch_add(1);
            } catch (const std::exception& e) {
                LOG_ERROR_FMT("线程 %zu 执行任务时发生异常: %s", threadId, e.what());
            } catch (...) {
                LOG_ERROR_FMT("线程 %zu 执行任务时发生未知异常", threadId);
            }
        } else {
            // 没有任务时等待
            std::unique_lock<std::mutex> lock(globalMutex_);
            condition_.wait_for(lock, std::chrono::milliseconds(1), [this] {
                return stop_.load() || getTotalQueueSize() > 0;
            });
        }
    }
    
    LOG_DEBUG_FMT("工作窃取线程 %zu 退出", threadId);
}

std::function<void()> WorkStealingThreadPool::getTask(size_t threadId) {
    // 首先尝试从自己的队列获取任务
    auto task = queues_[threadId]->popBack();
    if (task) {
        return task;
    }
    
    // 如果自己的队列为空，尝试窃取任务
    return stealTask(threadId);
}

std::function<void()> WorkStealingThreadPool::stealTask(size_t threadId) {
    totalStealsAttempted_.fetch_add(1);
    
    // 随机选择窃取目标，避免所有线程都从同一个队列窃取
    std::uniform_int_distribution<size_t> dist(0, queues_.size() - 1);
    
    // 尝试从随机选择的队列开始窃取
    size_t attempts = 0;
    const size_t maxAttempts = std::min(queues_.size(), static_cast<size_t>(4)); // 限制窃取尝试次数
    
    while (attempts < maxAttempts) {
        size_t targetId = dist(gen_);
        
        // 不从自己的队列窃取
        if (targetId == threadId) {
            targetId = (targetId + 1) % queues_.size();
        }
        
        auto task = queues_[targetId]->steal();
        if (task) {
            totalStealsSuccessful_.fetch_add(1);
            return task;
        }
        
        attempts++;
    }
    
    return nullptr; // 窃取失败
}

} // namespace utils
} // namespace dlogcover 