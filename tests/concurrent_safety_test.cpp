/**
 * @file concurrent_safety_test.cpp
 * @brief AST分析器并发安全性测试
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
#include <vector>
#include <memory>

using namespace dlogcover;
using namespace dlogcover::core::ast_analyzer;
using namespace dlogcover::source_manager;
using namespace dlogcover::config;

/**
 * @brief 并发安全测试类
 */
class ConcurrentSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        testDir_ = std::filesystem::temp_directory_path() / "dlogcover_concurrent_test";
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
        
        LOG_INFO_FMT("并发安全测试环境初始化完成，测试目录: %s", testDir_.c_str());
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
     * @brief 创建标准的C++测试内容
     * @param functionName 函数名
     * @return C++代码内容
     */
    std::string createStandardCppContent(const std::string& functionName) {
        return R"(
#include <iostream>
#include <vector>
#include <string>

void )" + functionName + R"(() {
    std::vector<std::string> data = {"hello", "world", "test"};
    for (const auto& item : data) {
        std::cout << "Processing: " << item << std::endl;
    }
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
};

/**
 * @brief 测试多个分析器实例并发访问
 */
TEST_F(ConcurrentSafetyTest, MultipleAnalyzerInstancesConcurrentAccess) {
    LOG_INFO("开始测试：多个分析器实例并发访问");
    
    // 创建测试文件
    std::vector<std::string> filePaths;
    for (int i = 1; i <= 5; ++i) {
        std::string filename = "concurrent_test_" + std::to_string(i) + ".cpp";
        std::string functionName = "concurrentFunction" + std::to_string(i);
        filePaths.push_back(createTestFile(filename, createStandardCppContent(functionName)));
    }
    
    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_TRUE(collectResult.isSuccess()) << "源文件收集失败: " << collectResult.errorMessage();
    
    const int numAnalyzers = 4;
    std::vector<std::future<bool>> futures;
    std::atomic<int> successCount{0};
    std::atomic<int> failureCount{0};
    
    auto startTime = std::chrono::steady_clock::now();
    
    // 启动多个分析器实例
    for (int i = 0; i < numAnalyzers; ++i) {
        futures.push_back(std::async(std::launch::async, [&, i]() {
            try {
                // 每个线程创建自己的分析器实例
                ASTAnalyzer analyzer(*config_, *sourceManager_, *configManager_);
                analyzer.setParallelMode(true, 2);
                
                LOG_INFO_FMT("分析器 %d 开始分析", i);
                
                auto result = analyzer.analyzeAllParallel();
                
                if (result.isSuccess()) {
                    successCount.fetch_add(1);
                    LOG_INFO_FMT("分析器 %d 分析成功", i);
                    return true;
                } else {
                    failureCount.fetch_add(1);
                    LOG_WARNING_FMT("分析器 %d 分析失败: %s", i, result.errorMessage().c_str());
                    return false;
                }
            } catch (const std::exception& e) {
                failureCount.fetch_add(1);
                LOG_ERROR_FMT("分析器 %d 异常: %s", i, e.what());
                return false;
            }
        }));
    }
    
    // 等待所有分析器完成
    bool allCompleted = true;
    for (auto& future : futures) {
        try {
            // 设置超时时间
            auto status = future.wait_for(std::chrono::seconds(30));
            if (status == std::future_status::timeout) {
                allCompleted = false;
                LOG_ERROR("分析器超时");
            } else {
                future.get();
            }
        } catch (const std::exception& e) {
            allCompleted = false;
            LOG_ERROR_FMT("等待分析器完成时异常: %s", e.what());
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_TRUE(allCompleted) << "并发分析器测试未全部完成";
    EXPECT_GT(successCount.load(), 0) << "没有分析器成功完成";
    
    LOG_INFO_FMT("并发分析器测试完成，耗时: %lld ms, 成功: %d, 失败: %d",
                duration.count(), successCount.load(), failureCount.load());
}

/**
 * @brief 测试高频率创建和销毁分析器
 */
TEST_F(ConcurrentSafetyTest, HighFrequencyCreateDestroy) {
    LOG_INFO("开始测试：高频率创建和销毁分析器");
    
    // 创建测试文件
    std::string filePath = createTestFile("frequency_test.cpp", 
                                         createStandardCppContent("frequencyTestFunction"));
    
    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_TRUE(collectResult.isSuccess()) << "源文件收集失败: " << collectResult.errorMessage();
    
    const int numIterations = 20;
    const int numThreads = 3;
    std::vector<std::future<int>> futures;
    std::atomic<int> totalCreated{0};
    std::atomic<int> totalDestroyed{0};
    
    auto startTime = std::chrono::steady_clock::now();
    
    // 启动多个线程，每个线程高频创建和销毁分析器
    for (int t = 0; t < numThreads; ++t) {
        futures.push_back(std::async(std::launch::async, [&, t]() {
            int localCreated = 0;
            int localDestroyed = 0;
            
            for (int i = 0; i < numIterations; ++i) {
                try {
                    // 创建分析器
                    auto analyzer = std::make_unique<ASTAnalyzer>(*config_, *sourceManager_, *configManager_);
                    analyzer->setParallelMode(false, 1);  // 使用串行模式减少复杂性
                    localCreated++;
                    totalCreated.fetch_add(1);
                    
                    LOG_DEBUG_FMT("线程 %d 创建分析器 %d", t, i);
                    
                    // 执行简单分析
                    auto result = analyzer->analyzeAllParallel();
                    
                    // 销毁分析器（通过reset）
                    analyzer.reset();
                    localDestroyed++;
                    totalDestroyed.fetch_add(1);
                    
                    LOG_DEBUG_FMT("线程 %d 销毁分析器 %d", t, i);
                    
                    // 短暂休眠模拟实际使用场景
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    
                } catch (const std::exception& e) {
                    LOG_ERROR_FMT("线程 %d 迭代 %d 异常: %s", t, i, e.what());
                }
            }
            
            return localCreated;
        }));
    }
    
    // 等待所有线程完成
    int totalExpected = 0;
    for (auto& future : futures) {
        try {
            auto status = future.wait_for(std::chrono::seconds(60));
            if (status == std::future_status::timeout) {
                LOG_ERROR("高频创建销毁测试超时");
            } else {
                totalExpected += future.get();
            }
        } catch (const std::exception& e) {
            LOG_ERROR_FMT("等待高频创建销毁测试完成时异常: %s", e.what());
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_EQ(totalCreated.load(), totalExpected) << "创建的分析器数量不匹配";
    EXPECT_EQ(totalDestroyed.load(), totalExpected) << "销毁的分析器数量不匹配";
    
    LOG_INFO_FMT("高频创建销毁测试完成，耗时: %lld ms, 创建: %d, 销毁: %d",
                duration.count(), totalCreated.load(), totalDestroyed.load());
}

/**
 * @brief 测试内存访问安全性
 */
TEST_F(ConcurrentSafetyTest, MemoryAccessSafety) {
    LOG_INFO("开始测试：内存访问安全性");
    
    // 创建多个测试文件
    std::vector<std::string> filePaths;
    for (int i = 1; i <= 8; ++i) {
        std::string filename = "memory_test_" + std::to_string(i) + ".cpp";
        std::string functionName = "memoryTestFunction" + std::to_string(i);
        filePaths.push_back(createTestFile(filename, createStandardCppContent(functionName)));
    }
    
    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_TRUE(collectResult.isSuccess()) << "源文件收集失败: " << collectResult.errorMessage();
    
    // 创建共享的分析器实例
    auto sharedAnalyzer = std::make_shared<ASTAnalyzer>(*config_, *sourceManager_, *configManager_);
    sharedAnalyzer->setParallelMode(true, 4);
    
    const int numReaders = 5;
    std::vector<std::future<bool>> futures;
    std::atomic<int> readOperations{0};
    std::atomic<bool> memoryCorruption{false};
    
    auto startTime = std::chrono::steady_clock::now();
    
    // 启动多个读取线程
    for (int i = 0; i < numReaders; ++i) {
        futures.push_back(std::async(std::launch::async, [&, i]() {
            try {
                for (int j = 0; j < 5; ++j) {
                    LOG_DEBUG_FMT("读取线程 %d 执行操作 %d", i, j);
                    
                    // 执行分析操作
                    auto result = sharedAnalyzer->analyzeAllParallel();
                    readOperations.fetch_add(1);
                    
                    // 检查结果的一致性
                    if (!result.isSuccess() && !result.errorMessage().empty()) {
                        // 分析可能失败，但错误信息应该是有效的
                        LOG_DEBUG_FMT("读取线程 %d 分析失败（可接受）: %s", i, result.errorMessage().c_str());
                    }
                    
                    // 短暂休眠
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                return true;
            } catch (const std::exception& e) {
                memoryCorruption.store(true);
                LOG_ERROR_FMT("读取线程 %d 异常（可能的内存问题）: %s", i, e.what());
                return false;
            }
        }));
    }
    
    // 等待所有读取线程完成
    bool allCompleted = true;
    for (auto& future : futures) {
        try {
            auto status = future.wait_for(std::chrono::seconds(45));
            if (status == std::future_status::timeout) {
                allCompleted = false;
                LOG_ERROR("内存访问安全测试超时");
            } else {
                bool success = future.get();
                if (!success) {
                    allCompleted = false;
                }
            }
        } catch (const std::exception& e) {
            allCompleted = false;
            LOG_ERROR_FMT("等待内存访问安全测试完成时异常: %s", e.what());
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_TRUE(allCompleted) << "内存访问安全测试未全部完成";
    EXPECT_FALSE(memoryCorruption.load()) << "检测到内存访问问题";
    EXPECT_GT(readOperations.load(), 0) << "没有执行读取操作";
    
    LOG_INFO_FMT("内存访问安全测试完成，耗时: %lld ms, 读取操作: %d",
                duration.count(), readOperations.load());
}

/**
 * @brief 测试资源竞争压力测试
 */
TEST_F(ConcurrentSafetyTest, ResourceContentionStressTest) {
    LOG_INFO("开始测试：资源竞争压力测试");
    
    // 创建大量测试文件
    std::vector<std::string> filePaths;
    for (int i = 1; i <= 15; ++i) {
        std::string filename = "stress_" + std::to_string(i) + ".cpp";
        std::string functionName = "stressFunction" + std::to_string(i);
        filePaths.push_back(createTestFile(filename, createStandardCppContent(functionName)));
    }
    
    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_TRUE(collectResult.isSuccess()) << "源文件收集失败: " << collectResult.errorMessage();
    
    const int numCompetitors = 6;
    std::vector<std::future<std::pair<bool, int>>> futures;
    std::atomic<int> totalAnalyses{0};
    std::atomic<int> successfulAnalyses{0};
    std::atomic<int> failedAnalyses{0};
    
    auto startTime = std::chrono::steady_clock::now();
    
    // 启动多个竞争线程
    for (int i = 0; i < numCompetitors; ++i) {
        futures.push_back(std::async(std::launch::async, [&, i]() {
            int localAnalyses = 0;
            bool allSuccess = true;
            
            try {
                // 每个线程执行多次分析
                for (int j = 0; j < 3; ++j) {
                    ASTAnalyzer analyzer(*config_, *sourceManager_, *configManager_);
                    analyzer.setParallelMode(true, 3);
                    
                    LOG_DEBUG_FMT("竞争线程 %d 执行分析 %d", i, j);
                    
                    auto result = analyzer.analyzeAllParallel();
                    localAnalyses++;
                    totalAnalyses.fetch_add(1);
                    
                    if (result.isSuccess()) {
                        successfulAnalyses.fetch_add(1);
                    } else {
                        failedAnalyses.fetch_add(1);
                        allSuccess = false;
                        LOG_WARNING_FMT("竞争线程 %d 分析 %d 失败: %s", i, j, result.errorMessage().c_str());
                    }
                    
                    // 模拟资源竞争
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            } catch (const std::exception& e) {
                allSuccess = false;
                LOG_ERROR_FMT("竞争线程 %d 异常: %s", i, e.what());
            }
            
            return std::make_pair(allSuccess, localAnalyses);
        }));
    }
    
    // 等待所有竞争线程完成
    int totalExpectedAnalyses = 0;
    bool anyThreadFailed = false;
    
    for (auto& future : futures) {
        try {
            auto status = future.wait_for(std::chrono::seconds(90));
            if (status == std::future_status::timeout) {
                anyThreadFailed = true;
                LOG_ERROR("资源竞争压力测试超时");
            } else {
                auto result = future.get();
                if (!result.first) {
                    anyThreadFailed = true;
                }
                totalExpectedAnalyses += result.second;
            }
        } catch (const std::exception& e) {
            anyThreadFailed = true;
            LOG_ERROR_FMT("等待资源竞争压力测试完成时异常: %s", e.what());
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    EXPECT_EQ(totalAnalyses.load(), totalExpectedAnalyses) << "分析次数不匹配";
    EXPECT_GT(successfulAnalyses.load(), 0) << "没有成功的分析";
    
    // 在高竞争环境下，允许一些失败，但不应该全部失败
    double successRate = static_cast<double>(successfulAnalyses.load()) / totalAnalyses.load();
    EXPECT_GT(successRate, 0.3) << "成功率过低，可能存在严重的资源竞争问题";
    
    LOG_INFO_FMT("资源竞争压力测试完成，耗时: %lld ms, 总分析: %d, 成功: %d, 失败: %d, 成功率: %.2f%%",
                duration.count(), totalAnalyses.load(), successfulAnalyses.load(), 
                failedAnalyses.load(), successRate * 100.0);
} 