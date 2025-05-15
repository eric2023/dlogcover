/**
 * @file reporter_test.cpp
 * @brief 报告生成器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/coverage/coverage_calculator.h>
#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/reporter/reporter.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>

namespace dlogcover {
namespace reporter {
namespace test {

// 创建测试目录和文件的助手函数
class ReporterTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试目录
        testDir_ = std::filesystem::temp_directory_path().string() + "/dlogcover_reporter_test";
        utils::FileUtils::createDirectory(testDir_);
        outputDir_ = testDir_ + "/output";
        utils::FileUtils::createDirectory(outputDir_);

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
        astAnalyzer_ = std::make_unique<core::ast_analyzer::ASTAnalyzer>(config_, *sourceManager_);

        // 分析所有文件
        ASSERT_TRUE(astAnalyzer_->analyzeAll()) << "分析所有文件失败";

        // 创建日志识别器
        logIdentifier_ = std::make_unique<core::log_identifier::LogIdentifier>(config_, *astAnalyzer_);

        // 识别日志调用
        ASSERT_TRUE(logIdentifier_->identifyLogCalls()) << "识别日志调用失败";

        // 创建覆盖率计算器
        coverageCalculator_ =
            std::make_unique<core::coverage::CoverageCalculator>(config_, *astAnalyzer_, *logIdentifier_);

        // 计算覆盖率
        ASSERT_TRUE(coverageCalculator_->calculate()) << "计算覆盖率失败";

        // 创建报告生成器
        reporter_ = std::make_unique<Reporter>(config_, *coverageCalculator_);
    }

    void TearDown() override {
        // 清理测试目录和数据
        std::filesystem::remove_all(testDir_);
        reporter_.reset();
        coverageCalculator_.reset();
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

        // 设置分析选项
        config.analysis.functionCoverage = true;
        config.analysis.branchCoverage = true;
        config.analysis.exceptionCoverage = true;
        config.analysis.keyPathCoverage = true;

        // 设置报告选项
        config.report.format = "text";  // 默认文本格式

        return config;
    }

    std::string testDir_;
    std::string outputDir_;
    config::Config config_;
    std::unique_ptr<source_manager::SourceManager> sourceManager_;
    std::unique_ptr<core::ast_analyzer::ASTAnalyzer> astAnalyzer_;
    std::unique_ptr<core::log_identifier::LogIdentifier> logIdentifier_;
    std::unique_ptr<core::coverage::CoverageCalculator> coverageCalculator_;
    std::unique_ptr<Reporter> reporter_;
};

// 测试初始化和销毁
TEST_F(ReporterTestFixture, InitializeAndDestroy) {
    // 这里主要测试构造和析构是否会导致崩溃
    SUCCEED();
}

// 测试文本格式报告生成
TEST_F(ReporterTestFixture, GenerateTextReport) {
    // 设置文本格式报告
    config_.report.format = "text";

    // 输出路径
    std::string outputPath = outputDir_ + "/coverage_report.txt";

    // 生成报告
    EXPECT_TRUE(reporter_->generateReport(outputPath)) << "生成文本报告失败";

    // 验证报告文件是否存在
    EXPECT_TRUE(utils::FileUtils::fileExists(outputPath)) << "报告文件不存在";

    // 读取文件内容进行简单验证
    std::string content;
    ASSERT_TRUE(utils::FileUtils::readFileToString(outputPath, content)) << "无法读取报告文件";

    // 验证报告内容
    EXPECT_TRUE(content.find("DLogCover 日志覆盖率报告") != std::string::npos) << "报告标题不正确";
    EXPECT_TRUE(content.find("总体覆盖率") != std::string::npos) << "缺少总体覆盖率部分";
    EXPECT_TRUE(content.find("文件覆盖率") != std::string::npos) << "缺少文件覆盖率部分";
    EXPECT_TRUE(content.find("改进建议") != std::string::npos) << "缺少改进建议部分";
}

// 测试JSON格式报告生成
TEST_F(ReporterTestFixture, GenerateJsonReport) {
    // 设置JSON格式报告
    config_.report.format = "json";

    // 输出路径
    std::string outputPath = outputDir_ + "/coverage_report.json";

    // 生成报告
    EXPECT_TRUE(reporter_->generateReport(outputPath)) << "生成JSON报告失败";

    // 验证报告文件是否存在
    EXPECT_TRUE(utils::FileUtils::fileExists(outputPath)) << "报告文件不存在";

    // 读取文件内容进行简单验证
    std::string content;
    ASSERT_TRUE(utils::FileUtils::readFileToString(outputPath, content)) << "无法读取报告文件";

    // 解析JSON
    bool validJson = true;
    try {
        nlohmann::json reportJson = nlohmann::json::parse(content);

        // 验证JSON结构
        EXPECT_TRUE(reportJson.find("metadata") != reportJson.end()) << "缺少元数据部分";
        EXPECT_TRUE(reportJson.find("overall") != reportJson.end()) << "缺少总体统计部分";
        EXPECT_TRUE(reportJson.find("files") != reportJson.end()) << "缺少文件统计部分";

        // 验证总体覆盖率指标
        EXPECT_TRUE(reportJson["overall"].find("function_coverage") != reportJson["overall"].end())
            << "缺少函数覆盖率指标";
        EXPECT_TRUE(reportJson["overall"].find("branch_coverage") != reportJson["overall"].end())
            << "缺少分支覆盖率指标";
        EXPECT_TRUE(reportJson["overall"].find("exception_coverage") != reportJson["overall"].end())
            << "缺少异常覆盖率指标";
        EXPECT_TRUE(reportJson["overall"].find("total_coverage") != reportJson["overall"].end())
            << "缺少总体覆盖率指标";

    } catch (const std::exception& e) {
        validJson = false;
        std::cerr << "JSON解析错误: " << e.what() << std::endl;
        FAIL() << "不是有效的JSON格式";
    }

    EXPECT_TRUE(validJson) << "报告不是有效的JSON格式";
}

}  // namespace test
}  // namespace reporter
}  // namespace dlogcover
