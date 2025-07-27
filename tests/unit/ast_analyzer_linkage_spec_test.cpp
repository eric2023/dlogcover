/**
 * @file ast_analyzer_linkage_spec_test.cpp
 * @brief AST分析器LinkageSpec处理测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/source_manager/source_manager.h>
#include <memory>
#include <string>

using namespace dlogcover::core::ast_analyzer;
using namespace dlogcover::config;
using namespace dlogcover::source_manager;

class ASTAnalyzerLinkageSpecTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建配置管理器
        configManager_ = std::make_unique<ConfigManager>();
        
        // 创建源文件管理器  
        sourceManager_ = std::make_unique<SourceManager>();
        
        // 创建AST分析器
        config_ = configManager_->getConfig();
        analyzer_ = std::make_unique<ASTAnalyzer>(config_, *sourceManager_, *configManager_);
    }

    std::unique_ptr<ConfigManager> configManager_;
    std::unique_ptr<SourceManager> sourceManager_;
    std::unique_ptr<ASTAnalyzer> analyzer_;
    Config config_;
};

/**
 * @brief 测试extern "C"块中函数的识别
 */
TEST_F(ASTAnalyzerLinkageSpecTest, ExternCFunctionRecognition) {
    // 创建测试文件内容
    std::string testCode = R"(
#include <cstdio>

extern "C" {
    void testFunction() {
        printf("test log\n");
    }
    
    int anotherFunction(int x) {
        printf("another log: %d\n", x);
        return x * 2;
    }
}
    )";
    
    // 将测试代码写入临时文件
    std::string tempFile = "/tmp/test_extern_c.cpp";
    std::ofstream ofs(tempFile);
    ofs << testCode;
    ofs.close();
    
    // 添加测试文件到源文件管理器
    sourceManager_->addFile(tempFile);
    
    // 分析文件
    auto result = analyzer_->analyzeFiles({tempFile});
    
    // 验证分析成功
    ASSERT_TRUE(result.hasError() == false) << "Analysis should succeed";
    
    // 获取分析结果
    auto nodeInfo = analyzer_->getASTNodeInfo(tempFile);
    ASSERT_NE(nodeInfo, nullptr) << "Should have analysis result";
    
    // 验证识别到了extern "C"块中的函数
    // 注意：这是一个基本的存在性测试，具体的函数计数验证需要更复杂的实现
    EXPECT_GT(nodeInfo->children.size(), 0) << "Should have child nodes";
    
    // 清理临时文件
    std::remove(tempFile.c_str());
}

/**
 * @brief 测试空的extern "C"块处理
 */
TEST_F(ASTAnalyzerLinkageSpecTest, EmptyExternCBlock) {
    std::string testCode = R"(
extern "C" {
    // 空的extern "C"块
}
    )";
    
    std::string tempFile = "/tmp/test_empty_extern_c.cpp";
    std::ofstream ofs(tempFile);
    ofs << testCode;
    ofs.close();
    
    sourceManager_->addFile(tempFile);
    auto result = analyzer_->analyzeFiles({tempFile});
    
    // 即使是空的extern "C"块，分析也应该成功
    EXPECT_TRUE(result.hasError() == false) << "Empty extern C block should not cause errors";
    
    std::remove(tempFile.c_str());
}

/**
 * @brief 测试嵌套的extern "C"块处理
 */
TEST_F(ASTAnalyzerLinkageSpecTest, NestedExternCBlocks) {
    std::string testCode = R"(
extern "C" {
    void outerFunction() {
        // 外层函数
    }
    
    extern "C" {
        void innerFunction() {
            // 内层函数
        }
    }
}
    )";
    
    std::string tempFile = "/tmp/test_nested_extern_c.cpp";
    std::ofstream ofs(tempFile);
    ofs << testCode;
    ofs.close();
    
    sourceManager_->addFile(tempFile);
    auto result = analyzer_->analyzeFiles({tempFile});
    
    // 嵌套的extern "C"块应该能正确处理
    EXPECT_TRUE(result.hasError() == false) << "Nested extern C blocks should be handled correctly";
    
    std::remove(tempFile.c_str());
} 