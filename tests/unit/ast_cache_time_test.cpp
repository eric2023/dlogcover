/**
 * @file ast_cache_time_test.cpp
 * @brief AST缓存时间处理单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/core/ast_analyzer/ast_cache.h>
#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace dlogcover {
namespace core {
namespace ast_analyzer {
namespace test {

class ASTCacheTimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        testDir_ = std::filesystem::temp_directory_path() / "dlogcover_cache_time_test";
        std::filesystem::create_directories(testDir_);
        
        // 创建AST缓存实例
        cache_ = std::make_unique<ASTCache>(10, 64); // 10个条目，64MB内存限制
        cache_->setDebugMode(true);
    }
    
    void TearDown() override {
        // 清理测试文件
        std::filesystem::remove_all(testDir_);
    }
    
    void createTestFile(const std::string& filename, const std::string& content) {
        std::filesystem::path filePath = testDir_ / filename;
        std::ofstream file(filePath);
        file << content;
        file.close();
    }
    
    void modifyTestFile(const std::string& filename, const std::string& newContent) {
        std::filesystem::path filePath = testDir_ / filename;
        std::ofstream file(filePath);
        file << newContent;
        file.close();
    }
    
    std::unique_ptr<ASTNodeInfo> createTestASTNode(const std::string& name) {
        auto node = std::make_unique<ASTNodeInfo>();
        node->name = name;
        node->text = "test content for " + name;
        node->type = NodeType::FUNCTION;
        return node;
    }
    
    std::filesystem::path testDir_;
    std::unique_ptr<ASTCache> cache_;
};

TEST_F(ASTCacheTimeTest, BasicCacheOperations) {
    std::string filename = "basic_test.cpp";
    std::string content = R"(
        #include <iostream>
        void testFunction() {
            std::cout << "test" << std::endl;
        }
    )";
    
    // 创建测试文件
    createTestFile(filename, content);
    std::filesystem::path filePath = testDir_ / filename;
    
    // 初始状态：缓存无效
    EXPECT_FALSE(cache_->isCacheValid(filePath.string())) 
        << "新文件的缓存应该无效";
    
    // 缓存AST信息
    auto astNode = createTestASTNode("testFunction");
    cache_->cacheAST(filePath.string(), std::move(astNode));
    
    // 验证缓存有效
    EXPECT_TRUE(cache_->isCacheValid(filePath.string())) 
        << "新缓存的文件应该是有效的";
    
    // 获取缓存的AST信息
    auto cachedAST = cache_->getCachedAST(filePath.string());
    EXPECT_NE(cachedAST, nullptr) << "应该能够获取缓存的AST信息";
    if (cachedAST) {
        EXPECT_EQ(cachedAST->name, "testFunction") << "缓存的AST节点名称应该正确";
    }
}

TEST_F(ASTCacheTimeTest, FileModificationDetection) {
    std::string filename = "modification_test.cpp";
    std::string originalContent = "void original() {}";
    
    // 创建并缓存文件
    createTestFile(filename, originalContent);
    std::filesystem::path filePath = testDir_ / filename;
    
    auto astNode = createTestASTNode("original");
    cache_->cacheAST(filePath.string(), std::move(astNode));
    
    // 验证初始缓存有效
    EXPECT_TRUE(cache_->isCacheValid(filePath.string())) 
        << "初始缓存应该有效";
    
    // 等待一小段时间确保时间戳不同
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // 修改文件内容
    std::string modifiedContent = "void modified() { int x = 42; }";
    modifyTestFile(filename, modifiedContent);
    
    // 验证缓存失效
    EXPECT_FALSE(cache_->isCacheValid(filePath.string())) 
        << "修改文件后缓存应该失效";
    
    // 验证无法获取过期的缓存
    auto cachedAST = cache_->getCachedAST(filePath.string());
    EXPECT_EQ(cachedAST, nullptr) << "修改文件后不应该能获取缓存的AST";
}

TEST_F(ASTCacheTimeTest, FileSizeChangeDetection) {
    std::string filename = "size_test.cpp";
    std::string shortContent = "void f() {}";
    
    // 创建短文件并缓存
    createTestFile(filename, shortContent);
    std::filesystem::path filePath = testDir_ / filename;
    
    auto astNode = createTestASTNode("f");
    cache_->cacheAST(filePath.string(), std::move(astNode));
    
    // 验证初始缓存有效
    EXPECT_TRUE(cache_->isCacheValid(filePath.string())) 
        << "初始缓存应该有效";
    
    // 修改为更长的内容
    std::string longContent = R"(
        #include <iostream>
        #include <vector>
        
        void expandedFunction() {
            std::vector<int> data;
            for (int i = 0; i < 100; ++i) {
                data.push_back(i);
            }
            
            for (const auto& item : data) {
                std::cout << item << std::endl;
            }
        }
    )";
    
    modifyTestFile(filename, longContent);
    
    // 验证文件大小变化导致缓存失效
    EXPECT_FALSE(cache_->isCacheValid(filePath.string())) 
        << "文件大小变化后缓存应该失效";
}

TEST_F(ASTCacheTimeTest, CacheStatisticsAccuracy) {
    std::string filename = "stats_test.cpp";
    std::string content = "void statsFunction() {}";
    
    createTestFile(filename, content);
    std::filesystem::path filePath = testDir_ / filename;
    
    // 初始统计
    EXPECT_EQ(cache_->getCacheHitCount(), 0) << "初始命中次数应该为0";
    EXPECT_EQ(cache_->getCacheMissCount(), 0) << "初始未命中次数应该为0";
    
    // 第一次访问（缓存未命中）
    EXPECT_FALSE(cache_->isCacheValid(filePath.string()));
    EXPECT_EQ(cache_->getCacheMissCount(), 1) << "第一次访问应该未命中";
    
    // 缓存AST信息
    auto astNode = createTestASTNode("statsFunction");
    cache_->cacheAST(filePath.string(), std::move(astNode));
    
    // 第二次访问（缓存命中）
    EXPECT_TRUE(cache_->isCacheValid(filePath.string()));
    auto cachedAST = cache_->getCachedAST(filePath.string());
    EXPECT_NE(cachedAST, nullptr);
    EXPECT_EQ(cache_->getCacheHitCount(), 1) << "第二次访问应该命中";
    
    // 验证命中率计算
    double expectedHitRate = 1.0 / 2.0; // 1次命中，2次总访问
    EXPECT_DOUBLE_EQ(cache_->getCacheHitRate(), expectedHitRate) 
        << "命中率计算应该正确";
}

TEST_F(ASTCacheTimeTest, FileSystemTimeTypePrecision) {
    std::string filename = "precision_test.cpp";
    std::string content = "void precisionTest() {}";
    
    createTestFile(filename, content);
    std::filesystem::path filePath = testDir_ / filename;
    
    // 获取文件的原始修改时间
    auto originalTime = std::filesystem::last_write_time(filePath);
    
    // 缓存文件
    auto astNode = createTestASTNode("precisionTest");
    cache_->cacheAST(filePath.string(), std::move(astNode));
    
    // 验证缓存有效
    EXPECT_TRUE(cache_->isCacheValid(filePath.string())) 
        << "缓存应该有效";
    
    // 验证时间精度：设置一个稍微不同的时间
    auto newTime = originalTime + std::chrono::milliseconds(1);
    std::filesystem::last_write_time(filePath, newTime);
    
    // 验证微小的时间变化被检测到
    EXPECT_FALSE(cache_->isCacheValid(filePath.string())) 
        << "微小的时间变化应该被检测到";
}

TEST_F(ASTCacheTimeTest, MultipleFilesCacheConsistency) {
    // 创建多个文件
    std::vector<std::string> filenames = {"file1.cpp", "file2.cpp", "file3.cpp"};
    
    for (size_t i = 0; i < filenames.size(); ++i) {
        std::string content = "void function" + std::to_string(i) + "() {}";
        
        createTestFile(filenames[i], content);
        std::filesystem::path filePath = testDir_ / filenames[i];
        
        auto astNode = createTestASTNode("function" + std::to_string(i));
        cache_->cacheAST(filePath.string(), std::move(astNode));
    }
    
    // 验证所有文件的缓存都有效
    for (const auto& filename : filenames) {
        std::filesystem::path filePath = testDir_ / filename;
        EXPECT_TRUE(cache_->isCacheValid(filePath.string())) 
            << filename << " 的缓存应该有效";
    }
    
    // 修改其中一个文件
    modifyTestFile(filenames[1], "void modifiedFunction() { /* changed */ }");
    
    // 验证只有修改的文件缓存失效
    EXPECT_TRUE(cache_->isCacheValid((testDir_ / filenames[0]).string())) 
        << filenames[0] << " 的缓存应该仍然有效";
    EXPECT_FALSE(cache_->isCacheValid((testDir_ / filenames[1]).string())) 
        << filenames[1] << " 的缓存应该失效";
    EXPECT_TRUE(cache_->isCacheValid((testDir_ / filenames[2]).string())) 
        << filenames[2] << " 的缓存应该仍然有效";
}

} // namespace test
} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover 