/**
 * @file thread_pool_test.cpp
 * @brief 线程池单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/utils/thread_pool.h>
#include "common/test_utils.h"
#include <atomic>
#include <chrono>
#include <vector>
#include <future>
#include <thread>

using namespace dlogcover::utils;
using namespace dlogcover::tests::common;

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试前的设置
    }

    void TearDown() override {
        // 测试后的清理
    }
};

// 测试线程池基本功能
TEST_F(ThreadPoolTest, BasicFunctionality) {
    ThreadPool pool(4);
    
    EXPECT_EQ(pool.getThreadCount(), 4);
    EXPECT_FALSE(pool.isStopped());
}

// 测试任务提交和执行
TEST_F(ThreadPoolTest, TaskSubmissionAndExecution) {
    ThreadPool pool(2);
    
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
    // 添加超时保护
    PerformanceTimer timer;
    const auto timeout = std::chrono::milliseconds(5000); // 5秒超时
    
    // 提交10个任务
    for (int i = 0; i < 10; ++i) {
        try {
            futures.emplace_back(pool.enqueue([&counter]() {
                counter.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }));
        } catch (const std::exception& e) {
            FAIL() << "Failed to enqueue task: " << e.what();
        }
    }
    
    // 等待所有任务完成，带超时检查
    for (auto& future : futures) {
        try {
            auto status = future.wait_for(std::chrono::milliseconds(1000));
            EXPECT_EQ(status, std::future_status::ready) << "Task did not complete within timeout";
            if (status == std::future_status::ready) {
                future.get(); // 获取结果以检查异常
            }
        } catch (const std::exception& e) {
            FAIL() << "Task execution failed: " << e.what();
        }
    }
    
    EXPECT_EQ(counter.load(), 10);
    EXPECT_LT(timer.elapsed().count(), timeout.count()) << "Test took too long to complete";
}

// 测试返回值
TEST_F(ThreadPoolTest, TaskReturnValues) {
    ThreadPool pool(3);
    
    try {
        auto future1 = pool.enqueue([]() { return 42; });
        auto future2 = pool.enqueue([]() { return std::string("hello"); });
        auto future3 = pool.enqueue([]() { return 3.14; });
        
        // 使用超时等待
        EXPECT_EQ(future1.wait_for(std::chrono::seconds(1)), std::future_status::ready);
        EXPECT_EQ(future2.wait_for(std::chrono::seconds(1)), std::future_status::ready);
        EXPECT_EQ(future3.wait_for(std::chrono::seconds(1)), std::future_status::ready);
        
        EXPECT_EQ(future1.get(), 42);
        EXPECT_EQ(future2.get(), "hello");
        EXPECT_NEAR_DOUBLE(future3.get(), 3.14);
    } catch (const std::exception& e) {
        FAIL() << "Task execution failed: " << e.what();
    }
}

// 测试异常处理
TEST_F(ThreadPoolTest, ExceptionHandling) {
    ThreadPool pool(2);
    
    try {
        auto future = pool.enqueue([]() {
            throw std::runtime_error("test exception");
            return 42;
        });
        
        EXPECT_EQ(future.wait_for(std::chrono::seconds(1)), std::future_status::ready);
        EXPECT_THROW(future.get(), std::runtime_error);
    } catch (const std::exception& e) {
        FAIL() << "Unexpected exception during test setup: " << e.what();
    }
}

// 测试线程池关闭
TEST_F(ThreadPoolTest, Shutdown) {
    auto pool = std::make_unique<ThreadPool>(2);
    
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
    try {
        // 提交一些任务
        for (int i = 0; i < 5; ++i) {
            futures.emplace_back(pool->enqueue([&counter]() {
                counter.fetch_add(1);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }));
        }
        
        // 关闭线程池
        pool->shutdown();
        EXPECT_TRUE(pool->isStopped());
        
        // 等待已提交的任务完成
        for (auto& future : futures) {
            auto status = future.wait_for(std::chrono::seconds(2));
            EXPECT_EQ(status, std::future_status::ready) << "Task did not complete after shutdown";
            if (status == std::future_status::ready) {
                future.get();
            }
        }
        
        EXPECT_EQ(counter.load(), 5);
        
        // 尝试提交新任务应该抛出异常
        EXPECT_THROW(pool->enqueue([]() { return 1; }), std::runtime_error);
    } catch (const std::exception& e) {
        FAIL() << "Shutdown test failed: " << e.what();
    }
}

// 测试自动线程数检测
TEST_F(ThreadPoolTest, AutoThreadDetection) {
    ThreadPool pool(0); // 0表示自动检测
    
    size_t expectedThreads = std::thread::hardware_concurrency();
    if (expectedThreads == 0) {
        expectedThreads = 4; // 默认值
    }
    
    EXPECT_EQ(pool.getThreadCount(), expectedThreads);
    EXPECT_GT(pool.getThreadCount(), 0) << "Thread count should be greater than 0";
}

// 测试并发性能
TEST_F(ThreadPoolTest, ConcurrentPerformance) {
    const int numTasks = 100;
    const int numThreads = 4;
    
    ThreadPool pool(numThreads);
    PerformanceTimer timer;
    
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
    try {
        for (int i = 0; i < numTasks; ++i) {
            futures.emplace_back(pool.enqueue([&counter]() {
                counter.fetch_add(1);
                // 模拟一些工作
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }));
        }
        
        // 等待所有任务完成
        for (auto& future : futures) {
            auto status = future.wait_for(std::chrono::seconds(5));
            EXPECT_EQ(status, std::future_status::ready) << "Performance test task timeout";
            if (status == std::future_status::ready) {
                future.get();
            }
        }
        
        auto duration = timer.elapsed();
        
        EXPECT_EQ(counter.load(), numTasks);
        
        // 并行执行应该比串行执行快
        // 这里只是一个粗略的检查，实际性能取决于硬件
        EXPECT_LT(duration.count(), numTasks * 2) << "Parallel execution should be faster than serial";
        
        // 确保测试在合理时间内完成
        EXPECT_LT(duration.count(), 10000) << "Performance test took too long: " << duration.count() << "ms";
    } catch (const std::exception& e) {
        FAIL() << "Performance test failed: " << e.what();
    }
}

// 测试队列大小监控
TEST_F(ThreadPoolTest, QueueSizeMonitoring) {
    ThreadPool pool(1); // 只有一个线程，容易观察队列
    
    std::atomic<bool> shouldContinue{true};
    
    try {
        // 提交一个长时间运行的任务
        auto longTask = pool.enqueue([&shouldContinue]() {
            while (shouldContinue.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
        
        // 等待一小段时间确保长任务开始执行
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // 提交更多任务到队列
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 5; ++i) {
            futures.emplace_back(pool.enqueue([i]() { return i; }));
        }
        
        // 检查队列大小
        EXPECT_GT(pool.getQueueSize(), 0) << "Queue should have pending tasks";
        
        // 停止长时间任务
        shouldContinue.store(false);
        
        // 等待长任务完成
        auto status = longTask.wait_for(std::chrono::seconds(2));
        EXPECT_EQ(status, std::future_status::ready) << "Long task did not complete";
        if (status == std::future_status::ready) {
            longTask.get();
        }
        
        // 等待其他任务完成
        for (auto& future : futures) {
            auto taskStatus = future.wait_for(std::chrono::seconds(1));
            EXPECT_EQ(taskStatus, std::future_status::ready) << "Queue task did not complete";
            if (taskStatus == std::future_status::ready) {
                future.get();
            }
        }
        
        // 队列应该为空
        EXPECT_EQ(pool.getQueueSize(), 0) << "Queue should be empty after all tasks complete";
    } catch (const std::exception& e) {
        shouldContinue.store(false); // 确保清理
        FAIL() << "Queue monitoring test failed: " << e.what();
    }
}

// 测试线程安全性
TEST_F(ThreadPoolTest, ThreadSafety) {
    ThreadPool pool(8);
    
    std::atomic<int> sharedCounter{0};
    std::vector<std::future<void>> futures;
    PerformanceTimer timer;
    
    try {
        // 多个线程同时修改共享变量
        for (int i = 0; i < 1000; ++i) {
            futures.emplace_back(pool.enqueue([&sharedCounter]() {
                for (int j = 0; j < 100; ++j) {
                    sharedCounter.fetch_add(1);
                }
            }));
        }
        
        // 等待所有任务完成
        for (auto& future : futures) {
            auto status = future.wait_for(std::chrono::seconds(10));
            EXPECT_EQ(status, std::future_status::ready) << "Thread safety test task timeout";
            if (status == std::future_status::ready) {
                future.get();
            }
        }
        
        EXPECT_EQ(sharedCounter.load(), 1000 * 100) << "Thread safety violation detected";
        
        // 确保测试在合理时间内完成
        EXPECT_LT(timer.elapsed().count(), 15000) << "Thread safety test took too long";
    } catch (const std::exception& e) {
        FAIL() << "Thread safety test failed: " << e.what();
    }
} 