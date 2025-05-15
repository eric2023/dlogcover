/**
 * @file ast_analyzer_test.cpp
 * @brief AST分析器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace core {
namespace ast_analyzer {
namespace test {

// 创建测试目录和文件的助手函数
class ASTAnalyzerTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试目录
        testDir_ = std::filesystem::temp_directory_path().string() + "/dlogcover_ast_test";
        utils::FileUtils::createDirectory(testDir_);

        // 创建测试文件
        createTestFile(testDir_ + "/test.cpp", R"(
#include <iostream>

// 简单函数
void test_function() {
    std::cout << "测试函数" << std::endl;
}

// 带条件分支的函数
int conditional_function(int value) {
    if (value > 0) {
        return value * 2;
    } else {
        return value * -1;
    }
}

// 带异常处理的函数
void exception_function() {
    try {
        throw std::runtime_error("测试异常");
    } catch (const std::exception& e) {
        std::cerr << "捕获异常: " << e.what() << std::endl;
    }
}

int main() {
    test_function();
    conditional_function(10);
    exception_function();
    return 0;
}
)");

        // 设置配置
        config_ = createTestConfig();

        // 初始化源文件管理器
        sourceManager_ = std::make_unique<source_manager::SourceManager>(config_);

        // 收集源文件
        ASSERT_TRUE(sourceManager_->collectSourceFiles()) << "收集源文件失败";

        // 创建AST分析器
        astAnalyzer_ = std::make_unique<ASTAnalyzer>(config_, *sourceManager_);
    }

    void TearDown() override {
        // 清理测试目录和数据
        std::filesystem::remove_all(testDir_);
        astAnalyzer_.reset();
        sourceManager_.reset();
    }

    void createTestFile(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        file << content;
        file.close();
    }

    config::Config createTestConfig() {
        config::Config config;

        // 设置扫描目录
        config.scan.directories = {testDir_};

        // 设置文件类型
        config.scan.fileTypes = {".cpp", ".h", ".hpp", ".cc", ".c"};

        // 设置日志函数
        config.logFunctions.qt.enabled = true;
        config.logFunctions.qt.functions = {"qDebug", "qInfo", "qWarning", "qCritical", "qFatal"};

        return config;
    }

    std::string testDir_;
    config::Config config_;
    std::unique_ptr<source_manager::SourceManager> sourceManager_;
    std::unique_ptr<ASTAnalyzer> astAnalyzer_;
};

// 测试初始化和销毁
TEST_F(ASTAnalyzerTestFixture, InitializeAndDestroy) {
    // 这里主要测试构造和析构是否会导致崩溃
    SUCCEED();
}

// 测试分析单个文件
TEST_F(ASTAnalyzerTestFixture, AnalyzeSingleFile) {
    // 获取测试文件路径
    std::string testFilePath = testDir_ + "/test.cpp";

    // 分析单个文件
    EXPECT_TRUE(astAnalyzer_->analyze(testFilePath)) << "分析文件失败";

    // 验证AST节点信息
    const ASTNodeInfo* nodeInfo = astAnalyzer_->getASTNodeInfo(testFilePath);
    ASSERT_NE(nullptr, nodeInfo) << "未生成AST节点信息";
}

// 测试分析所有文件
TEST_F(ASTAnalyzerTestFixture, AnalyzeAllFiles) {
    // 分析所有文件
    EXPECT_TRUE(astAnalyzer_->analyzeAll()) << "分析所有文件失败";

    // 验证是否分析了所有文件
    const auto& allASTNodeInfo = astAnalyzer_->getAllASTNodeInfo();
    EXPECT_EQ(1, allASTNodeInfo.size()) << "分析文件数量不符";

    // 验证是否包含测试文件
    std::string testFilePath = testDir_ + "/test.cpp";
    EXPECT_NE(allASTNodeInfo.find(testFilePath), allASTNodeInfo.end()) << "未找到测试文件的AST节点";
}

}  // namespace test
}  // namespace ast_analyzer
}  // namespace core
}  // namespace dlogcover
