/**
 * @file thread_pool_test.cpp
 * @brief 线程池单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/utils/thread_pool.h>
#include <atomic>
#include <chrono>
#include <vector>
#include <future>

using namespace dlogcover::utils;

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
    
    // 提交10个任务
    for (int i = 0; i < 10; ++i) {
        futures.emplace_back(pool.enqueue([&counter]() {
            counter.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }));
    }
    
    // 等待所有任务完成
    for (auto& future : futures) {
        future.get();
    }
    
    EXPECT_EQ(counter.load(), 10);
}

// 测试返回值
TEST_F(ThreadPoolTest, TaskReturnValues) {
    ThreadPool pool(3);
    
    auto future1 = pool.enqueue([]() { return 42; });
    auto future2 = pool.enqueue([]() { return std::string("hello"); });
    auto future3 = pool.enqueue([]() { return 3.14; });
    
    EXPECT_EQ(future1.get(), 42);
    EXPECT_EQ(future2.get(), "hello");
    EXPECT_DOUBLE_EQ(future3.get(), 3.14);
}

// 测试异常处理
TEST_F(ThreadPoolTest, ExceptionHandling) {
    ThreadPool pool(2);
    
    auto future = pool.enqueue([]() {
        throw std::runtime_error("test exception");
        return 42;
    });
    
    EXPECT_THROW(future.get(), std::runtime_error);
}

// 测试线程池关闭
TEST_F(ThreadPoolTest, Shutdown) {
    auto pool = std::make_unique<ThreadPool>(2);
    
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
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
        future.get();
    }
    
    EXPECT_EQ(counter.load(), 5);
    
    // 尝试提交新任务应该抛出异常
    EXPECT_THROW(pool->enqueue([]() { return 1; }), std::runtime_error);
}

// 测试自动线程数检测
TEST_F(ThreadPoolTest, AutoThreadDetection) {
    ThreadPool pool(0); // 0表示自动检测
    
    size_t expectedThreads = std::thread::hardware_concurrency();
    if (expectedThreads == 0) {
        expectedThreads = 4; // 默认值
    }
    
    EXPECT_EQ(pool.getThreadCount(), expectedThreads);
}

// 测试并发性能
TEST_F(ThreadPoolTest, ConcurrentPerformance) {
    const int numTasks = 100;
    const int numThreads = 4;
    
    ThreadPool pool(numThreads);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < numTasks; ++i) {
        futures.emplace_back(pool.enqueue([&counter]() {
            counter.fetch_add(1);
            // 模拟一些工作
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }));
    }
    
    // 等待所有任务完成
    for (auto& future : futures) {
        future.get();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(counter.load(), numTasks);
    
    // 并行执行应该比串行执行快
    // 这里只是一个粗略的检查，实际性能取决于硬件
    EXPECT_LT(duration.count(), numTasks * 2); // 应该比串行快很多
}

// 测试队列大小监控
TEST_F(ThreadPoolTest, QueueSizeMonitoring) {
    ThreadPool pool(1); // 只有一个线程，容易观察队列
    
    std::atomic<bool> shouldContinue{true};
    
    // 提交一个长时间运行的任务
    auto longTask = pool.enqueue([&shouldContinue]() {
        while (shouldContinue.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // 提交更多任务到队列
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.emplace_back(pool.enqueue([i]() { return i; }));
    }
    
    // 检查队列大小
    EXPECT_GT(pool.getQueueSize(), 0);
    
    // 停止长时间任务
    shouldContinue.store(false);
    longTask.get();
    
    // 等待其他任务完成
    for (auto& future : futures) {
        future.get();
    }
    
    // 队列应该为空
    EXPECT_EQ(pool.getQueueSize(), 0);
}

// 测试线程安全性
TEST_F(ThreadPoolTest, ThreadSafety) {
    ThreadPool pool(8);
    
    std::atomic<int> sharedCounter{0};
    std::vector<std::future<void>> futures;
    
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
        future.get();
    }
    
    EXPECT_EQ(sharedCounter.load(), 1000 * 100);
} 