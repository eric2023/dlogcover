/**
 * @file thread_pool.cpp
 * @brief 线程池实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/utils/thread_pool.h>
#include <dlogcover/utils/log_utils.h>
#include <algorithm>

namespace dlogcover {
namespace utils {

ThreadPool::ThreadPool(size_t numThreads) {
    // 如果未指定线程数，使用硬件并发数
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) {
            numThreads = 4; // 默认4个线程
        }
    }
    
    // 限制最大线程数
    numThreads = std::min(numThreads, static_cast<size_t>(32));
    
    LOG_INFO_FMT("初始化线程池，线程数: %zu", numThreads);
    
    // 创建工作线程
    workers_.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this, i] {
            LOG_DEBUG_FMT("工作线程 %zu 启动", i);
            
            while (true) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    
                    // 等待任务或停止信号
                    condition_.wait(lock, [this] {
                        return stop_.load() || !tasks_.empty();
                    });
                    
                    // 如果收到停止信号且任务队列为空，退出
                    if (stop_.load() && tasks_.empty()) {
                        LOG_DEBUG_FMT("工作线程 %zu 退出", i);
                        return;
                    }
                    
                    // 获取任务
                    if (!tasks_.empty()) {
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                }
                
                // 执行任务
                if (task) {
                    try {
                        task();
                    } catch (const std::exception& e) {
                        LOG_ERROR_FMT("线程 %zu 执行任务时发生异常: %s", i, e.what());
                    } catch (...) {
                        LOG_ERROR_FMT("线程 %zu 执行任务时发生未知异常", i);
                    }
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::shutdown() {
    if (stop_.load()) {
        return; // 已经停止
    }
    
    LOG_INFO("关闭线程池");
    
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
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
    
    // 清空任务队列
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        std::queue<std::function<void()>> empty;
        tasks_.swap(empty);
    }
    
    LOG_INFO("线程池已关闭");
}

size_t ThreadPool::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return tasks_.size();
}

} // namespace utils
} // namespace dlogcover 