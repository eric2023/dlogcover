/**
 * @file ast_cache_test.cpp
 * @brief AST缓存单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/core/ast_analyzer/ast_cache.h>
#include "common/test_utils.h"
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <iostream>

using namespace dlogcover::core::ast_analyzer;
using namespace dlogcover::tests::common;

class ASTCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 使用跨平台兼容的临时目录管理器
        tempDirManager_ = std::make_unique<TempDirectoryManager>("dlogcover_cache_test");
        
        // 创建测试文件
        testFile1_ = tempDirManager_->createTestFile("test1.cpp", "int main() { return 0; }");
        testFile2_ = tempDirManager_->createTestFile("test2.cpp", "void func() { /* test */ }");
        
        cache_ = std::make_unique<ASTCache>(5, 10); // 小缓存用于测试
    }

    void TearDown() override {
        cache_.reset();
        tempDirManager_.reset(); // 自动清理临时文件
    }

    std::unique_ptr<ASTNodeInfo> createTestASTInfo(const std::string& text) {
        auto astInfo = std::make_unique<ASTNodeInfo>();
        astInfo->type = NodeType::FUNCTION;
        astInfo->name = "test_function";
        astInfo->text = text;
        astInfo->hasLogging = false;
        astInfo->location.line = 1;
        astInfo->location.column = 1;
        astInfo->endLocation.line = 1;
        astInfo->endLocation.column = static_cast<unsigned int>(text.length());
        return astInfo;
    }

protected:
    std::unique_ptr<TempDirectoryManager> tempDirManager_;
    std::filesystem::path testFile1_;
    std::filesystem::path testFile2_;
    std::unique_ptr<ASTCache> cache_;
};

// 测试基本缓存功能
TEST_F(ASTCacheTest, BasicCaching) {
    std::string filePath = testFile1_.string();
    auto astInfo = createTestASTInfo("test function");
    
    // 初始状态：缓存为空
    EXPECT_FALSE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCachedAST(filePath), nullptr);
    EXPECT_EQ(cache_->getCurrentSize(), 0);
    
    // 缓存AST信息
    cache_->cacheAST(filePath, std::move(astInfo));
    
    // 验证缓存
    EXPECT_TRUE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCurrentSize(), 1);
    
    auto cachedAST = cache_->getCachedAST(filePath);
    ASSERT_NE(cachedAST, nullptr);
    EXPECT_EQ(cachedAST->text, "test function");
    EXPECT_EQ(cachedAST->type, NodeType::FUNCTION);
    EXPECT_EQ(cachedAST->name, "test_function");
}

// 测试文件变化检测
TEST_F(ASTCacheTest, FileChangeDetection) {
    std::string filePath = testFile1_.string();
    auto astInfo = createTestASTInfo("original content");
    
    // 缓存原始内容
    cache_->cacheAST(filePath, std::move(astInfo));
    EXPECT_TRUE(cache_->isCacheValid(filePath));
    
    // 等待一小段时间确保时间戳不同
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 修改文件
    tempDirManager_->createTestFile("test1.cpp", "modified content");
    
    // 验证缓存失效
    EXPECT_FALSE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCachedAST(filePath), nullptr);
}

// 测试LRU淘汰策略
TEST_F(ASTCacheTest, LRUEviction) {
    // 填满缓存（最大5个条目）
    std::vector<std::filesystem::path> testFiles;
    for (int i = 0; i < 5; ++i) {
        std::string filename = "test" + std::to_string(i) + ".cpp";
        std::string content = "content " + std::to_string(i);
        auto filePath = tempDirManager_->createTestFile(filename, content);
        testFiles.push_back(filePath);
        
        auto astInfo = createTestASTInfo(content);
        cache_->cacheAST(filePath.string(), std::move(astInfo));
    }
    
    EXPECT_EQ(cache_->getCurrentSize(), 5);
    
    // 添加第6个条目，应该触发LRU淘汰
    auto newFilePath = tempDirManager_->createTestFile("test_new.cpp", "new content");
    auto newAstInfo = createTestASTInfo("new content");
    cache_->cacheAST(newFilePath.string(), std::move(newAstInfo));
    
    // 缓存大小应该仍然是5
    EXPECT_EQ(cache_->getCurrentSize(), 5);
    
    // 新文件应该在缓存中
    EXPECT_TRUE(cache_->isCacheValid(newFilePath.string()));
}

// 测试缓存统计
TEST_F(ASTCacheTest, CacheStatistics) {
    std::string filePath = testFile1_.string();
    auto astInfo = createTestASTInfo("test content");
    
    // 初始统计
    EXPECT_EQ(cache_->getCacheHitCount(), 0);
    EXPECT_EQ(cache_->getCacheMissCount(), 0);
    EXPECT_NEAR_DOUBLE(cache_->getCacheHitRate(), 0.0);
    
    // 缓存未命中
    EXPECT_FALSE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCacheMissCount(), 1);
    
    // 缓存AST
    cache_->cacheAST(filePath, std::move(astInfo));
    
    // 缓存命中
    EXPECT_TRUE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCacheHitCount(), 1);
    EXPECT_EQ(cache_->getCacheMissCount(), 1);
    EXPECT_NEAR_DOUBLE(cache_->getCacheHitRate(), 0.5);
    
    // 再次命中
    EXPECT_TRUE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCacheHitCount(), 2);
    EXPECT_RELATIVE_EQUAL(cache_->getCacheHitRate(), 2.0/3.0, 1e-6);
}

// 测试缓存清空功能
TEST_F(ASTCacheTest, TestClearCacheFunction) {
    // 使用fixture中的缓存实例
    std::string filePath = testFile1_.string();
    auto astInfo = createTestASTInfo("test content");
    
    // 添加一些内容到缓存
    cache_->cacheAST(filePath, std::move(astInfo));
    EXPECT_EQ(cache_->getCurrentSize(), 1);
    
    // 触发一次缓存命中
    EXPECT_TRUE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCacheHitCount(), 1);
    EXPECT_EQ(cache_->getCacheMissCount(), 0);
    
    // 清空缓存
    cache_->clearCache();
    
    // 验证缓存已清空且统计信息已重置
    EXPECT_EQ(cache_->getCurrentSize(), 0);
    EXPECT_EQ(cache_->getCacheHitCount(), 0);
    EXPECT_EQ(cache_->getCacheMissCount(), 0);
    
    // 验证缓存确实无效（这会增加未命中计数）
    EXPECT_FALSE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCacheMissCount(), 1);
}

// 测试空AST信息处理
TEST_F(ASTCacheTest, NullASTHandling) {
    std::string filePath = testFile1_.string();
    
    // 尝试缓存空的AST信息
    cache_->cacheAST(filePath, nullptr);
    
    // 缓存应该为空
    EXPECT_EQ(cache_->getCurrentSize(), 0);
    EXPECT_FALSE(cache_->isCacheValid(filePath));
}

// 测试内存使用估算
TEST_F(ASTCacheTest, MemoryUsageEstimation) {
    std::string filePath = testFile1_.string();
    auto astInfo = createTestASTInfo("test content for memory estimation");
    
    // 初始内存使用应该为0
    EXPECT_EQ(cache_->getEstimatedMemoryUsage(), 0);
    
    // 缓存AST信息
    cache_->cacheAST(filePath, std::move(astInfo));
    
    // 内存使用应该大于0
    EXPECT_GT(cache_->getEstimatedMemoryUsage(), 0);
    
    // 添加更多条目，内存使用应该增加
    size_t initialMemory = cache_->getEstimatedMemoryUsage();
    
    std::string filePath2 = testFile2_.string();
    auto astInfo2 = createTestASTInfo("another test content");
    cache_->cacheAST(filePath2, std::move(astInfo2));
    
    EXPECT_GT(cache_->getEstimatedMemoryUsage(), initialMemory);
}

// 测试调试模式
TEST_F(ASTCacheTest, DebugMode) {
    // 启用调试模式
    cache_->setDebugMode(true);
    
    std::string filePath = testFile1_.string();
    auto astInfo = createTestASTInfo("debug test content");
    
    // 这些操作在调试模式下应该产生额外的日志输出
    // 但在单元测试中我们主要验证功能正确性
    cache_->cacheAST(filePath, std::move(astInfo));
    EXPECT_TRUE(cache_->isCacheValid(filePath));
    
    // 禁用调试模式
    cache_->setDebugMode(false);
    
    // 功能应该仍然正常
    EXPECT_TRUE(cache_->isCacheValid(filePath));
}

// 测试统计信息字符串
TEST_F(ASTCacheTest, StatisticsString) {
    std::string filePath = testFile1_.string();
    auto astInfo = createTestASTInfo("statistics test");
    
    // 获取初始统计信息
    std::string initialStats = cache_->getStatistics();
    EXPECT_FALSE(initialStats.empty());
    EXPECT_NE(initialStats.find("AST缓存统计信息"), std::string::npos);
    
    // 添加缓存条目
    cache_->cacheAST(filePath, std::move(astInfo));
    cache_->isCacheValid(filePath); // 触发命中
    
    // 获取更新后的统计信息
    std::string updatedStats = cache_->getStatistics();
    EXPECT_FALSE(updatedStats.empty());
    EXPECT_NE(updatedStats.find("缓存条目数: 1"), std::string::npos);
    EXPECT_NE(updatedStats.find("缓存命中: 1"), std::string::npos);
}

// 性能测试：大量缓存操作
TEST_F(ASTCacheTest, PerformanceTest) {
    const int numOperations = 100; // 减少操作数量以提高测试速度
    auto largeCache = std::make_unique<ASTCache>(numOperations, 100);
    
    PerformanceTimer timer;
    
    // 创建临时目录管理器用于性能测试
    auto perfTempDir = std::make_unique<TempDirectoryManager>("dlogcover_perf_test");
    
    try {
        // 执行大量缓存操作
        for (int i = 0; i < numOperations; ++i) {
            std::string filename = "perf_test_" + std::to_string(i) + ".cpp";
            std::string content = "performance test content " + std::to_string(i);
            auto filePath = perfTempDir->createTestFile(filename, content);
            
            auto astInfo = createTestASTInfo("performance test " + std::to_string(i));
            largeCache->cacheAST(filePath.string(), std::move(astInfo));
            
            // 每10个操作检查一次缓存
            if (i % 10 == 0) {
                largeCache->isCacheValid(filePath.string());
            }
        }
        
        auto duration = timer.elapsed();
        
        // 性能要求：操作应该在合理时间内完成
        EXPECT_LT(duration.count(), 5000) << "Performance test took too long: " << duration.count() << "ms";
        
        // 验证最终状态
        EXPECT_EQ(largeCache->getCurrentSize(), numOperations);
        EXPECT_GT(largeCache->getCacheHitCount(), 0);
        
        // 验证内存使用合理
        size_t memoryUsage = largeCache->getEstimatedMemoryUsage();
        EXPECT_GT(memoryUsage, 0) << "Memory usage should be greater than 0";
        EXPECT_LT(memoryUsage, 100 * 1024 * 1024) << "Memory usage should be less than 100MB";
        
    } catch (const std::exception& e) {
        FAIL() << "Performance test failed: " << e.what();
    }
} 