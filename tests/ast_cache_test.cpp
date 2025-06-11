/**
 * @file ast_cache_test.cpp
 * @brief AST缓存单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/core/ast_analyzer/ast_cache.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

using namespace dlogcover::core::ast_analyzer;

class ASTCacheTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试目录
        testDir_ = std::filesystem::temp_directory_path() / "dlogcover_cache_test";
        std::filesystem::create_directories(testDir_);
        
        // 创建测试文件
        testFile1_ = testDir_ / "test1.cpp";
        testFile2_ = testDir_ / "test2.cpp";
        
        createTestFile(testFile1_, "int main() { return 0; }");
        createTestFile(testFile2_, "void func() { /* test */ }");
        
        cache_ = std::make_unique<ASTCache>(5, 10); // 小缓存用于测试
    }

    void TearDown() override {
        cache_.reset();
        std::filesystem::remove_all(testDir_);
    }

    void createTestFile(const std::filesystem::path& path, const std::string& content) {
        std::ofstream file(path);
        file << content;
        file.close();
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
    std::filesystem::path testDir_;
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
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // 修改文件
    createTestFile(testFile1_, "modified content");
    
    // 验证缓存失效
    EXPECT_FALSE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCachedAST(filePath), nullptr);
}

// 测试LRU淘汰策略
TEST_F(ASTCacheTest, LRUEviction) {
    // 填满缓存（最大5个条目）
    for (int i = 0; i < 5; ++i) {
        std::string filePath = (testDir_ / ("test" + std::to_string(i) + ".cpp")).string();
        createTestFile(filePath, "content " + std::to_string(i));
        
        auto astInfo = createTestASTInfo("content " + std::to_string(i));
        cache_->cacheAST(filePath, std::move(astInfo));
    }
    
    EXPECT_EQ(cache_->getCurrentSize(), 5);
    
    // 添加第6个条目，应该触发LRU淘汰
    std::string newFilePath = (testDir_ / "test_new.cpp").string();
    createTestFile(newFilePath, "new content");
    auto newAstInfo = createTestASTInfo("new content");
    cache_->cacheAST(newFilePath, std::move(newAstInfo));
    
    // 缓存大小应该仍然是5
    EXPECT_EQ(cache_->getCurrentSize(), 5);
    
    // 新文件应该在缓存中
    EXPECT_TRUE(cache_->isCacheValid(newFilePath));
}

// 测试缓存统计
TEST_F(ASTCacheTest, CacheStatistics) {
    std::string filePath = testFile1_.string();
    auto astInfo = createTestASTInfo("test content");
    
    // 初始统计
    EXPECT_EQ(cache_->getCacheHitCount(), 0);
    EXPECT_EQ(cache_->getCacheMissCount(), 0);
    EXPECT_EQ(cache_->getCacheHitRate(), 0.0);
    
    // 缓存未命中
    EXPECT_FALSE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCacheMissCount(), 1);
    
    // 缓存AST
    cache_->cacheAST(filePath, std::move(astInfo));
    
    // 缓存命中
    EXPECT_TRUE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCacheHitCount(), 1);
    EXPECT_EQ(cache_->getCacheMissCount(), 1);
    EXPECT_EQ(cache_->getCacheHitRate(), 0.5);
    
    // 再次命中
    EXPECT_TRUE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCacheHitCount(), 2);
    EXPECT_EQ(cache_->getCacheHitRate(), 2.0/3.0);
}

// 测试缓存清空
TEST_F(ASTCacheTest, CacheClear) {
    std::string filePath = testFile1_.string();
    auto astInfo = createTestASTInfo("test content");
    
    // 添加缓存条目
    cache_->cacheAST(filePath, std::move(astInfo));
    EXPECT_EQ(cache_->getCurrentSize(), 1);
    EXPECT_TRUE(cache_->isCacheValid(filePath));
    
    // 清空缓存
    cache_->clearCache();
    
    // 验证缓存已清空
    EXPECT_EQ(cache_->getCurrentSize(), 0);
    EXPECT_FALSE(cache_->isCacheValid(filePath));
    EXPECT_EQ(cache_->getCacheHitCount(), 0);
    EXPECT_EQ(cache_->getCacheMissCount(), 0);
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
    const int numOperations = 1000;
    auto largeCache = std::make_unique<ASTCache>(numOperations, 100);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 执行大量缓存操作
    for (int i = 0; i < numOperations; ++i) {
        std::string filePath = (testDir_ / ("perf_test_" + std::to_string(i) + ".cpp")).string();
        createTestFile(filePath, "performance test content " + std::to_string(i));
        
        auto astInfo = createTestASTInfo("performance test " + std::to_string(i));
        largeCache->cacheAST(filePath, std::move(astInfo));
        
        // 每10个操作检查一次缓存
        if (i % 10 == 0) {
            largeCache->isCacheValid(filePath);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 性能要求：1000次操作应该在合理时间内完成（比如5秒）
    EXPECT_LT(duration.count(), 5000);
    
    // 验证最终状态
    EXPECT_EQ(largeCache->getCurrentSize(), numOperations);
    EXPECT_GT(largeCache->getCacheHitCount(), 0);
} 