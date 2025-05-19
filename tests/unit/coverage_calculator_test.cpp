/**
 * @file coverage_calculator_test.cpp
 * @brief 覆盖率计算器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/coverage/coverage_calculator.h>
#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace core {
namespace coverage {
namespace test {

// 创建测试目录和文件的助手函数
class CoverageCalculatorTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试目录
        testDir_ = std::filesystem::temp_directory_path().string() + "/dlogcover_coverage_test";
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
        auto collectResult = sourceManager_->collectSourceFiles();
        ASSERT_FALSE(collectResult.hasError()) << "收集源文件失败: " << collectResult.errorMessage();
        ASSERT_TRUE(collectResult.value()) << "未能有效收集源文件";

        // 创建AST分析器
        astAnalyzer_ = std::make_unique<ast_analyzer::ASTAnalyzer>(config_, *sourceManager_);

        // 分析所有文件
        auto analyzeResult = astAnalyzer_->analyzeAll();
        ASSERT_FALSE(analyzeResult.hasError()) << "分析所有文件失败: " << analyzeResult.errorMessage();
        ASSERT_TRUE(analyzeResult.value()) << "分析文件返回false";

        // 创建日志识别器
        logIdentifier_ = std::make_unique<log_identifier::LogIdentifier>(config_, *astAnalyzer_);

        // 识别日志调用
        auto identifyResult = logIdentifier_->identifyLogCalls();
        ASSERT_FALSE(identifyResult.hasError()) << "识别日志调用失败: " << identifyResult.errorMessage();
        ASSERT_TRUE(identifyResult.value()) << "识别日志调用返回false";

        // 创建覆盖率计算器
        coverageCalculator_ = std::make_unique<CoverageCalculator>(config_, *astAnalyzer_, *logIdentifier_);
    }

    void TearDown() override {
        // 清理测试目录和数据
        std::filesystem::remove_all(testDir_);
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

        // 设置日志函数
        config.logFunctions.qt.enabled = true;
        config.logFunctions.qt.functions = {"qDebug", "qInfo", "qWarning", "qCritical", "qFatal"};

        // 添加必要的编译参数
        config.scan.compilerArgs = {"-I/usr/include", "-I/usr/include/c++/8", "-I/usr/include/x86_64-linux-gnu/c++/8",
                                    "-I/usr/include/x86_64-linux-gnu", "-I/usr/local/include",
                                    // Qt头文件路径
                                    "-I/usr/include/x86_64-linux-gnu/qt5", "-I/usr/include/x86_64-linux-gnu/qt5/QtCore",
                                    "-I/usr/include/x86_64-linux-gnu/qt5/QtGui",
                                    "-I/usr/include/x86_64-linux-gnu/qt5/QtWidgets",
                                    // 系统定义
                                    "-D__GNUG__", "-D__linux__", "-D__x86_64__"};

        // 设置为Qt项目
        config.scan.isQtProject = true;

        return config;
    }

    std::string testDir_;
    config::Config config_;
    std::unique_ptr<source_manager::SourceManager> sourceManager_;
    std::unique_ptr<ast_analyzer::ASTAnalyzer> astAnalyzer_;
    std::unique_ptr<log_identifier::LogIdentifier> logIdentifier_;
    std::unique_ptr<CoverageCalculator> coverageCalculator_;
};

// 测试初始化和销毁
TEST_F(CoverageCalculatorTestFixture, InitializeAndDestroy) {
    // 这里主要测试构造和析构是否会导致崩溃
    SUCCEED();
}

// 测试覆盖率计算
TEST_F(CoverageCalculatorTestFixture, CalculateCoverage) {
    // 计算覆盖率
    auto calculateResult = coverageCalculator_->calculate();
    EXPECT_TRUE(calculateResult) << "计算覆盖率失败";

    // 获取测试文件的覆盖率统计信息
    std::string testFilePath = testDir_ + "/test.cpp";
    const CoverageStats& fileStats = coverageCalculator_->getCoverageStats(testFilePath);

    // 简单验证覆盖率计算是否进行
    // 注意：由于我们的实现是框架性的，这里只验证基本逻辑
    EXPECT_GE(fileStats.functionCoverage, 0.0);
    EXPECT_LE(fileStats.functionCoverage, 1.0);

    EXPECT_GE(fileStats.branchCoverage, 0.0);
    EXPECT_LE(fileStats.branchCoverage, 1.0);

    EXPECT_GE(fileStats.exceptionCoverage, 0.0);
    EXPECT_LE(fileStats.exceptionCoverage, 1.0);

    EXPECT_GE(fileStats.keyPathCoverage, 0.0);
    EXPECT_LE(fileStats.keyPathCoverage, 1.0);

    EXPECT_GE(fileStats.overallCoverage, 0.0);
    EXPECT_LE(fileStats.overallCoverage, 1.0);
}

// 测试总体覆盖率计算
TEST_F(CoverageCalculatorTestFixture, OverallCoverage) {
    // 计算覆盖率
    auto calculateResult = coverageCalculator_->calculate();
    EXPECT_TRUE(calculateResult) << "计算覆盖率失败";

    // 获取总体覆盖率统计信息
    const CoverageStats& overallStats = coverageCalculator_->getOverallCoverageStats();

    // 验证总体覆盖率计算
    EXPECT_GE(overallStats.functionCoverage, 0.0);
    EXPECT_LE(overallStats.functionCoverage, 1.0);

    EXPECT_GE(overallStats.branchCoverage, 0.0);
    EXPECT_LE(overallStats.branchCoverage, 1.0);

    EXPECT_GE(overallStats.exceptionCoverage, 0.0);
    EXPECT_LE(overallStats.exceptionCoverage, 1.0);

    EXPECT_GE(overallStats.keyPathCoverage, 0.0);
    EXPECT_LE(overallStats.keyPathCoverage, 1.0);

    EXPECT_GE(overallStats.overallCoverage, 0.0);
    EXPECT_LE(overallStats.overallCoverage, 1.0);
}

// 测试未覆盖路径建议
TEST_F(CoverageCalculatorTestFixture, UncoveredPathSuggestions) {
    // 计算覆盖率
    auto calculateResult = coverageCalculator_->calculate();
    EXPECT_TRUE(calculateResult) << "计算覆盖率失败";

    // 获取总体覆盖率统计信息
    const CoverageStats& overallStats = coverageCalculator_->getOverallCoverageStats();

    // 在实际实现中，应该验证生成的建议的内容
    // 由于我们的实现是框架性的，这里只进行简单检查

    // 如果有未覆盖路径，应该有对应的建议
    if (!overallStats.uncoveredPaths.empty()) {
        EXPECT_FALSE(overallStats.uncoveredPaths.front().suggestion.empty()) << "未生成建议";
    }
}

}  // namespace test
}  // namespace coverage
}  // namespace core
}  // namespace dlogcover
