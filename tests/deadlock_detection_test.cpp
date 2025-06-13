/**
 * @file deadlock_detection_test.cpp
 * @brief AST分析器死锁检测测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/config/config.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/utils/log_utils.h>
#include <thread>
#include <chrono>
#include <future>
#include <atomic>
#include <filesystem>
#include <fstream>

using namespace dlogcover;
using namespace dlogcover::core::ast_analyzer;
using namespace dlogcover::source_manager;
using namespace dlogcover::config;

/**
 * @brief 死锁检测器类
 * 
 * 提供超时监控机制，防止测试因死锁而永久挂起
 */
class DeadlockDetector {
public:
    /**
     * @brief 构造函数
     * @param timeoutMs 超时时间（毫秒）
     */
    explicit DeadlockDetector(int timeoutMs = 30000) : timeoutMs_(timeoutMs) {}

    /**
     * @brief 执行带超时检测的函数
     * @tparam Func 函数类型
     * @param func 要执行的函数
     * @return 执行结果，超时返回false
     */
    template<typename Func>
    bool executeWithTimeout(Func&& func) {
        std::atomic<bool> completed{false};
        std::atomic<bool> hasDeadlock{false};
        
        // 启动监控线程
        auto monitor = std::thread([&]() {
            auto start = std::chrono::steady_clock::now();
            while (!completed.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start);
                
                if (elapsed.count() > timeoutMs_) {
                    hasDeadlock.store(true);
                    LOG_ERROR_FMT("检测到潜在死锁，测试超时: %lld ms", elapsed.count());
                    break;
                }
            }
        });

        // 执行目标函数
        try {
            func();
            completed.store(true);
        } catch (...) {
            completed.store(true);
            monitor.join();
            throw;
        }

        monitor.join();
        return !hasDeadlock.load();
    }

private:
    int timeoutMs_;
};

/**
 * @brief 死锁检测测试类
 */
class DeadlockDetectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        testDir_ = std::filesystem::temp_directory_path() / "dlogcover_deadlock_test";
        std::filesystem::create_directories(testDir_);
        
        // 初始化配置管理器
        configManager_ = std::make_unique<ConfigManager>();
        configManager_->initializeDefault(testDir_.string());
        
        // 获取配置并设置项目目录
        config_ = &configManager_->getConfig();
        const_cast<Config&>(*config_).project.directory = testDir_.string();
        const_cast<Config&>(*config_).scan.directories = {testDir_.string()};
        const_cast<Config&>(*config_).scan.file_extensions = {".cpp", ".h"};
        
        // 初始化源文件管理器
        sourceManager_ = std::make_unique<SourceManager>(*config_);
        
        LOG_INFO_FMT("测试环境初始化完成，测试目录: %s", testDir_.c_str());
    }
    
    void TearDown() override {
        // 清理测试目录
        std::error_code ec;
        std::filesystem::remove_all(testDir_, ec);
        if (ec) {
            LOG_WARNING_FMT("清理测试目录失败: %s", ec.message().c_str());
        }
    }
    
    /**
     * @brief 创建测试文件
     * @param filename 文件名
     * @param content 文件内容
     * @return 文件完整路径
     */
    std::string createTestFile(const std::string& filename, const std::string& content) {
        auto filePath = testDir_ / filename;
        std::ofstream file(filePath);
        file << content;
        file.close();
        
        LOG_DEBUG_FMT("创建测试文件: %s", filePath.c_str());
        return filePath.string();
    }
    
    /**
     * @brief 创建简单的C++测试文件内容
     * @param functionName 函数名
     * @return 文件内容
     */
    std::string createSimpleCppContent(const std::string& functionName = "testFunction") {
        return R"(
#include <iostream>

void )" + functionName + R"(() {
    std::cout << "Hello from )" + functionName + R"(" << std::endl;
    // 简单的日志调用
    qDebug() << "Debug message";
}

int main() {
    )" + functionName + R"(();
    return 0;
}
)";
    }
    
protected:
    std::filesystem::path testDir_;
    std::unique_ptr<ConfigManager> configManager_;
    const Config* config_;
    std::unique_ptr<SourceManager> sourceManager_;
    DeadlockDetector detector_;
};

/**
 * @brief 测试单文件并行处理（验证递归调用问题）
 */
TEST_F(DeadlockDetectionTest, SingleFileParallelProcessing) {
    LOG_INFO("开始测试：单文件并行处理");
    
    // 创建单个测试文件
    std::string testContent = R"(
#include <iostream>
#include <vector>

void testFunction() {
    std::vector<int> data = {1, 2, 3, 4, 5};
    for (const auto& item : data) {
        std::cout << "Processing: " << item << std::endl;
    }
}

int main() {
    testFunction();
    return 0;
}
)";
    
    std::string filePath = createTestFile("single_test.cpp", testContent);
    
    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_TRUE(collectResult.isSuccess()) << "源文件收集失败: " << collectResult.errorMessage();
    
    // 创建AST分析器并启用并行模式
    ASTAnalyzer analyzer(*config_, *sourceManager_, *configManager_);
    analyzer.setParallelMode(true, 4);  // 设置多线程，但只有一个文件
    
    bool testCompleted = false;
    auto start = std::chrono::steady_clock::now();
    
    // 使用死锁检测器执行分析
    bool noDeadlock = detector_.executeWithTimeout([&]() {
        auto result = analyzer.analyzeAllParallel();
        testCompleted = true;
        
        // 验证结果
        EXPECT_TRUE(result.isSuccess()) << "单文件并行分析失败: " << result.errorMessage();
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        LOG_INFO_FMT("单文件分析完成，耗时: %lld ms", duration.count());
    });
    
    EXPECT_TRUE(noDeadlock) << "检测到死锁或超时";
    EXPECT_TRUE(testCompleted) << "测试未完成";
}

/**
 * @brief 测试少量文件并行处理
 */
TEST_F(DeadlockDetectionTest, FewFilesParallelProcessing) {
    LOG_INFO("开始测试：少量文件并行处理");
    
    // 创建3个测试文件
    std::vector<std::string> filePaths;
    for (int i = 1; i <= 3; ++i) {
        std::string content = R"(
#include <iostream>
#include <string>

void function)" + std::to_string(i) + R"(() {
    std::string message = "Hello from file )" + std::to_string(i) + R"(";
    std::cout << message << std::endl;
}
)";
        std::string filename = "test_file_" + std::to_string(i) + ".cpp";
        filePaths.push_back(createTestFile(filename, content));
    }
    
    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_TRUE(collectResult.isSuccess()) << "源文件收集失败: " << collectResult.errorMessage();
    
    LOG_INFO_FMT("开始分析 %zu 个文件...", filePaths.size());
    
    // 创建AST分析器
    ASTAnalyzer analyzer(*config_, *sourceManager_, *configManager_);
    analyzer.setParallelMode(true, 8);  // 线程数大于文件数
    
    bool testCompleted = false;
    
    // 使用死锁检测器执行分析
    bool noDeadlock = detector_.executeWithTimeout([&]() {
        auto result = analyzer.analyzeAllParallel();
        testCompleted = true;
        
        // 验证结果
        EXPECT_TRUE(result.isSuccess()) << "少量文件并行分析失败: " << result.errorMessage();
    });
    
    EXPECT_TRUE(noDeadlock) << "检测到死锁或超时";
    EXPECT_TRUE(testCompleted) << "测试未完成";
}

/**
 * @brief 测试线程数限制场景
 */
TEST_F(DeadlockDetectionTest, ThreadLimitScenario) {
    LOG_INFO("开始测试：线程数限制场景");
    
    // 创建5个测试文件
    std::vector<std::string> filePaths;
    for (int i = 1; i <= 5; ++i) {
        std::string content = R"(
#include <vector>
#include <algorithm>

void processData)" + std::to_string(i) + R"(() {
    std::vector<int> data;
    for (int j = 0; j < 100; ++j) {
        data.push_back(j * )" + std::to_string(i) + R"();
    }
    std::sort(data.begin(), data.end());
}
)";
        std::string filename = "limit_test_" + std::to_string(i) + ".cpp";
        filePaths.push_back(createTestFile(filename, content));
    }
    
    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_TRUE(collectResult.isSuccess()) << "源文件收集失败: " << collectResult.errorMessage();
    
    // 测试不同的线程数限制
    std::vector<size_t> threadCounts = {1, 2, 3, 4};
    
    for (size_t threadCount : threadCounts) {
        LOG_INFO_FMT("测试线程数: %zu", threadCount);
        
        ASTAnalyzer analyzer(*config_, *sourceManager_, *configManager_);
        analyzer.setParallelMode(true, threadCount);
        
        bool testCompleted = false;
        
        // 使用死锁检测器执行分析
        bool noDeadlock = detector_.executeWithTimeout([&]() {
            auto result = analyzer.analyzeAllParallel();
            testCompleted = true;
            
            // 验证结果
            EXPECT_TRUE(result.isSuccess()) << "线程限制场景分析失败: " << result.errorMessage();
        });
        
        EXPECT_TRUE(noDeadlock) << "线程数 " << threadCount << " 时检测到死锁或超时";
        EXPECT_TRUE(testCompleted) << "线程数 " << threadCount << " 时测试未完成";
    }
}

/**
 * @brief 测试并发互斥锁
 */
TEST_F(DeadlockDetectionTest, ConcurrentMutexTest) {
    LOG_INFO("开始测试：并发互斥锁");
    
    // 创建多个测试文件用于高并发测试
    std::vector<std::string> filePaths;
    for (int i = 1; i <= 10; ++i) {
        std::string content = R"(
#include <mutex>
#include <thread>
#include <vector>

class ThreadSafeCounter)" + std::to_string(i) + R"( {
private:
    mutable std::mutex mutex_;
    int count_ = 0;
    
public:
    void increment() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++count_;
    }
    
    int get() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return count_;
    }
};
)";
        std::string filename = "mutex_test_" + std::to_string(i) + ".cpp";
        filePaths.push_back(createTestFile(filename, content));
    }
    
    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_TRUE(collectResult.isSuccess()) << "源文件收集失败: " << collectResult.errorMessage();
    
    // 创建多个分析器实例并发执行
    const int numAnalyzers = 3;
    std::vector<std::future<bool>> futures;
    
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < numAnalyzers; ++i) {
        futures.push_back(std::async(std::launch::async, [&, i]() {
            ASTAnalyzer analyzer(*config_, *sourceManager_, *configManager_);
            analyzer.setParallelMode(true, 4);
            
            auto result = analyzer.analyzeAllParallel();
            return result.isSuccess();
        }));
    }
    
    // 等待所有分析器完成
    bool allSuccess = true;
    for (auto& future : futures) {
        try {
            bool success = future.get();
            allSuccess = allSuccess && success;
        } catch (const std::exception& e) {
            allSuccess = false;
            LOG_WARNING_FMT("并发分析异常: %s", e.what());
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_TRUE(allSuccess) << "并发互斥锁测试失败";
    
    LOG_INFO_FMT("高并发分析完成，耗时: %lld ms", duration.count());
}

/**
 * @brief 测试异常情况下的资源清理
 */
TEST_F(DeadlockDetectionTest, ExceptionResourceCleanup) {
    LOG_INFO("开始测试：异常情况下的资源清理");
    
    // 创建包含语法错误的文件
    std::string invalidContent = R"(
#include <iostream>

// 故意的语法错误
void invalidFunction( {
    std::cout << "This will cause parse error" << std::endl;
    // 缺少右括号
}

int main() {
    invalidFunction();
    return 0;
}
)";
    
    std::string filePath = createTestFile("invalid_syntax.cpp", invalidContent);
    
    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_TRUE(collectResult.isSuccess()) << "源文件收集失败: " << collectResult.errorMessage();
    
    // 创建AST分析器
    ASTAnalyzer analyzer(*config_, *sourceManager_, *configManager_);
    analyzer.setParallelMode(true, 2);
    
    bool testCompleted = false;
    auto start = std::chrono::steady_clock::now();
    
    // 使用死锁检测器执行分析
    bool noDeadlock = detector_.executeWithTimeout([&]() {
        try {
            auto result = analyzer.analyzeAllParallel();
            testCompleted = true;
            
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            LOG_INFO_FMT("异常情况分析完成，耗时: %lld ms, 结果: %s",
                        duration.count(), result.isSuccess() ? "成功" : "失败");
            
        } catch (const std::exception& e) {
            testCompleted = true;
            LOG_WARNING_FMT("异常情况测试捕获异常（预期行为）: %s", e.what());
        }
    });
    
    EXPECT_TRUE(noDeadlock) << "异常情况下检测到死锁或超时";
    EXPECT_TRUE(testCompleted) << "异常情况测试未完成";
}

/**
 * @brief 压力测试：大量小文件
 */
TEST_F(DeadlockDetectionTest, StressTestManySmallFiles) {
    LOG_INFO("开始测试：压力测试 - 大量小文件");
    
    // 创建20个小文件
    std::vector<std::string> filePaths;
    for (int i = 1; i <= 20; ++i) {
        std::string content = R"(
#include <iostream>

void smallFunction)" + std::to_string(i) + R"(() {
    std::cout << "Small function )" + std::to_string(i) + R"(" << std::endl;
}
)";
        std::string filename = "small_" + std::to_string(i) + ".cpp";
        filePaths.push_back(createTestFile(filename, content));
    }
    
    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_TRUE(collectResult.isSuccess()) << "源文件收集失败: " << collectResult.errorMessage();
    
    LOG_INFO_FMT("开始压力测试，分析 %zu 个文件...", filePaths.size());
    
    // 创建AST分析器
    ASTAnalyzer analyzer(*config_, *sourceManager_, *configManager_);
    analyzer.setParallelMode(true, 8);
    
    bool testCompleted = false;
    auto start = std::chrono::steady_clock::now();
    
    // 使用更长的超时时间进行压力测试
    DeadlockDetector stressDetector(60000);  // 60秒超时
    
    bool noDeadlock = stressDetector.executeWithTimeout([&]() {
        auto result = analyzer.analyzeAllParallel();
        testCompleted = true;
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // 验证结果
        EXPECT_TRUE(result.isSuccess()) << "压力测试失败: " << result.errorMessage();
        
        LOG_INFO_FMT("压力测试完成，耗时: %lld ms", duration.count());
    });
    
    EXPECT_TRUE(noDeadlock) << "压力测试中检测到死锁或超时";
    EXPECT_TRUE(testCompleted) << "压力测试未完成";
} 