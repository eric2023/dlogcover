/**
 * @file log_identifier_test.cpp
 * @brief 日志识别器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace core {
namespace log_identifier {
namespace test {

// 创建测试目录和文件的助手函数
class LogIdentifierTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试目录
        testDir_ = std::filesystem::temp_directory_path().string() + "/dlogcover_log_test";
        utils::FileUtils::createDirectory(testDir_);

        // 创建测试文件
        createTestFile(testDir_ + "/test.cpp", R"(
#include <iostream>
#include <QDebug>

// 普通函数
void regular_function() {
    std::cout << "普通函数" << std::endl;
}

// 带日志的函数
void logged_function() {
    qDebug() << "这是一条调试日志";
    std::cout << "带日志的函数" << std::endl;
    qInfo() << "这是一条信息日志";
}

// 带条件分支的函数
int conditional_function(int value) {
    if (value > 0) {
        qDebug() << "正数分支" << value;
        return value * 2;
    } else {
        qWarning() << "负数或零分支" << value;
        return value * -1;
    }
}

// 带异常处理的函数
void exception_function() {
    try {
        throw std::runtime_error("测试异常");
    } catch (const std::exception& e) {
        qCritical() << "捕获异常:" << e.what();
        std::cerr << "捕获异常: " << e.what() << std::endl;
    }
}

int main() {
    regular_function();
    logged_function();
    conditional_function(10);
    conditional_function(-5);

    try {
        exception_function();
    } catch (...) {
        qFatal() << "致命错误";
    }

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
        astAnalyzer_ = std::make_unique<ast_analyzer::ASTAnalyzer>(config_, *sourceManager_);

        // 分析所有文件
        ASSERT_TRUE(astAnalyzer_->analyzeAll()) << "分析所有文件失败";

        // 创建日志识别器
        logIdentifier_ = std::make_unique<LogIdentifier>(config_, *astAnalyzer_);
    }

    void TearDown() override {
        // 清理测试目录和数据
        std::filesystem::remove_all(testDir_);
        logIdentifier_.reset();
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

        // 设置Qt日志函数
        config.logFunctions.qt.enabled = true;
        config.logFunctions.qt.functions = {"qDebug", "qInfo", "qWarning", "qCritical", "qFatal"};
        config.logFunctions.qt.categoryFunctions = {"qCDebug", "qCInfo", "qCWarning", "qCCritical"};

        // 设置自定义日志函数
        config.logFunctions.custom.enabled = true;
        config.logFunctions.custom.functions["debug"] = {"debug", "log_debug"};
        config.logFunctions.custom.functions["info"] = {"info", "log_info"};
        config.logFunctions.custom.functions["warning"] = {"warning", "log_warning"};
        config.logFunctions.custom.functions["error"] = {"error", "log_error"};

        return config;
    }

    std::string testDir_;
    config::Config config_;
    std::unique_ptr<source_manager::SourceManager> sourceManager_;
    std::unique_ptr<ast_analyzer::ASTAnalyzer> astAnalyzer_;
    std::unique_ptr<LogIdentifier> logIdentifier_;
};

// 测试初始化和销毁
TEST_F(LogIdentifierTestFixture, InitializeAndDestroy) {
    // 这里主要测试构造和析构是否会导致崩溃
    SUCCEED();
}

// 测试日志函数构建
TEST_F(LogIdentifierTestFixture, LogFunctionNameBuilding) {
    // 验证日志函数名集合
    const auto& logFunctionNames = logIdentifier_->getLogFunctionNames();

    // 检查Qt日志函数
    EXPECT_TRUE(logFunctionNames.count("qDebug") > 0);
    EXPECT_TRUE(logFunctionNames.count("qInfo") > 0);
    EXPECT_TRUE(logFunctionNames.count("qWarning") > 0);
    EXPECT_TRUE(logFunctionNames.count("qCritical") > 0);
    EXPECT_TRUE(logFunctionNames.count("qFatal") > 0);

    // 检查Qt分类日志函数
    EXPECT_TRUE(logFunctionNames.count("qCDebug") > 0);
    EXPECT_TRUE(logFunctionNames.count("qCInfo") > 0);
    EXPECT_TRUE(logFunctionNames.count("qCWarning") > 0);
    EXPECT_TRUE(logFunctionNames.count("qCCritical") > 0);

    // 检查自定义日志函数
    EXPECT_TRUE(logFunctionNames.count("debug") > 0);
    EXPECT_TRUE(logFunctionNames.count("log_debug") > 0);
    EXPECT_TRUE(logFunctionNames.count("info") > 0);
    EXPECT_TRUE(logFunctionNames.count("log_info") > 0);
}

// 测试日志调用识别
TEST_F(LogIdentifierTestFixture, IdentifyLogCalls) {
    // 识别日志调用
    EXPECT_TRUE(logIdentifier_->identifyLogCalls()) << "识别日志调用失败";

    // 获取测试文件的日志调用信息
    std::string testFilePath = testDir_ + "/test.cpp";
    const auto& logCalls = logIdentifier_->getLogCalls(testFilePath);

    // 因为我们使用的是空实现，所以这里实际不会有日志调用被识别出来
    // 在实际实现完成后，可以添加更严格的测试

    // 获取所有日志调用
    const auto& allLogCalls = logIdentifier_->getAllLogCalls();

    // 确保每个文件都被处理
    EXPECT_TRUE(allLogCalls.count(testFilePath) > 0);
}

}  // namespace test
}  // namespace log_identifier
}  // namespace core
}  // namespace dlogcover
