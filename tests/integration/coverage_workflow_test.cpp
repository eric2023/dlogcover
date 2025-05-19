/**
 * @file coverage_workflow_test.cpp
 * @brief 覆盖率工作流集成测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/command_line_parser.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/coverage/coverage_calculator.h>
#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

#include "../common/test_utils.h"

namespace dlogcover {
namespace test {

class CoverageWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_dir_ = TestUtils::createTestTempDir("coverage_test_");
        ASSERT_FALSE(test_dir_.empty());

        // 初始化日志系统
        log_file_ = test_dir_ + "/test.log";
        utils::Logger::init(log_file_, true, utils::LogLevel::INFO);

        // 创建测试源文件目录
        source_dir_ = test_dir_ + "/src";
        std::filesystem::create_directories(source_dir_);

        // 创建配置
        config_ = TestUtils::createTestConfig(test_dir_);
    }

    void TearDown() override {
        // 关闭日志系统
        utils::Logger::shutdown();

        // 清理临时目录
        if (!test_dir_.empty()) {
            TestUtils::cleanupTestTempDir(test_dir_);
        }
    }

    // 创建测试源文件
    std::string createTestSource(const std::string& filename, const std::string& content) {
        std::string file_path = source_dir_ + "/" + filename;
        std::ofstream source_file(file_path);
        EXPECT_TRUE(source_file.is_open());
        source_file << content;
        source_file.close();
        return file_path;
    }

protected:
    std::string test_dir_;
    std::string log_file_;
    std::string source_dir_;
    config::Config config_;
};

// 测试基本覆盖率计算
TEST_F(CoverageWorkflowTest, BasicCoverageCalculation) {
    // 创建测试源文件
    std::string source = R"(
#include <QDebug>
#include <stdexcept>

class Calculator {
public:
    int add(int a, int b) {
        qDebug() << "Adding" << a << "and" << b;
        return a + b;
    }

    int divide(int a, int b) {
        if (b == 0) {
            qCritical() << "Division by zero!";
            throw std::invalid_argument("Division by zero");
        }
        qDebug() << "Dividing" << a << "by" << b;
        return a / b;
    }
};

int main() {
    Calculator calc;
    calc.add(5, 3);
    try {
        calc.divide(10, 2);
        calc.divide(10, 0);
    } catch(const std::exception& e) {
        qCritical() << "Caught exception:" << e.what();
    }
    return 0;
}
    )";

    std::string source_path = createTestSource("calculator.cpp", source);

    // 初始化源文件管理器
    auto source_manager = TestUtils::createTestSourceManager(config_);

    // 收集源文件
    auto collect_result = source_manager->collectSourceFiles();
    ASSERT_FALSE(collect_result.hasError()) << "收集源文件失败: " << collect_result.errorMessage();
    ASSERT_TRUE(collect_result.value()) << "未能有效收集源文件";

    // 创建AST分析器
    core::ast_analyzer::ASTAnalyzer ast_analyzer(config_, *source_manager);

    // 分析所有文件
    auto analyze_result = ast_analyzer.analyzeAll();
    ASSERT_FALSE(analyze_result.hasError()) << "分析所有文件失败: " << analyze_result.errorMessage();
    ASSERT_TRUE(analyze_result.value()) << "分析文件返回false";

    // 创建日志识别器
    core::log_identifier::LogIdentifier log_identifier(config_, ast_analyzer);

    // 识别日志调用
    auto identify_result = log_identifier.identifyLogCalls();
    ASSERT_FALSE(identify_result.hasError()) << "识别日志调用失败: " << identify_result.errorMessage();
    ASSERT_TRUE(identify_result.value()) << "识别日志调用返回false";

    // 创建覆盖率计算器
    core::coverage::CoverageCalculator coverage_calculator(config_, ast_analyzer, log_identifier);

    // 计算覆盖率
    ASSERT_TRUE(coverage_calculator.calculate()) << "计算覆盖率失败";

    // 获取测试文件的覆盖率统计信息
    const auto& file_stats = coverage_calculator.getCoverageStats(source_path);

    // 验证覆盖率计算是否进行
    EXPECT_GE(file_stats.functionCoverage, 0.0);
    EXPECT_LE(file_stats.functionCoverage, 1.0);
    EXPECT_GE(file_stats.branchCoverage, 0.0);
    EXPECT_LE(file_stats.branchCoverage, 1.0);
    EXPECT_GE(file_stats.exceptionCoverage, 0.0);
    EXPECT_LE(file_stats.exceptionCoverage, 1.0);
}

// 测试复杂场景覆盖率
TEST_F(CoverageWorkflowTest, ComplexCoverageCalculation) {
    // 创建测试源文件
    std::string source = R"(
#include <QDebug>
#include <vector>
#include <stdexcept>

class DataProcessor {
public:
    void process(const std::vector<int>& data) {
        qDebug() << "Starting data processing";

        try {
            if (data.empty()) {
                qWarning() << "Empty data set";
                return;
            }

            for (size_t i = 0; i < data.size(); ++i) {
                if (data[i] < 0) {
                    qCritical() << "Negative value at index" << i;
                    throw std::invalid_argument("Negative values not allowed");
                }

                if (data[i] % 2 == 0) {
                    qDebug() << "Processing even number:" << data[i];
                    processEven(data[i]);
                } else {
                    qDebug() << "Processing odd number:" << data[i];
                    processOdd(data[i]);
                }
            }

            qInfo() << "Processing completed successfully";
        } catch (const std::exception& e) {
            qCritical() << "Processing failed:" << e.what();
            throw;
        }
    }

private:
    void processEven(int value) {
        qDebug() << "Even number processing";
        if (value == 0) {
            qWarning() << "Special case: zero";
        }
    }

    void processOdd(int value) {
        qDebug() << "Odd number processing";
        if (value > 100) {
            qWarning() << "Large odd number:" << value;
        }
    }
};

int main() {
    DataProcessor processor;
    std::vector<int> data = {2, 5, 0, 10, -1};
    try {
        processor.process(data);
    } catch(const std::exception& e) {
        qCritical() << "Main caught exception:" << e.what();
    }
    return 0;
}
    )";

    std::string source_path = createTestSource("data_processor.cpp", source);

    // 初始化源文件管理器
    auto source_manager = TestUtils::createTestSourceManager(config_);

    // 收集源文件
    auto collect_result = source_manager->collectSourceFiles();
    ASSERT_FALSE(collect_result.hasError()) << "收集源文件失败: " << collect_result.errorMessage();
    ASSERT_TRUE(collect_result.value()) << "未能有效收集源文件";

    // 创建AST分析器
    core::ast_analyzer::ASTAnalyzer ast_analyzer(config_, *source_manager);

    // 分析所有文件
    auto analyze_result = ast_analyzer.analyzeAll();
    ASSERT_FALSE(analyze_result.hasError()) << "分析所有文件失败: " << analyze_result.errorMessage();
    ASSERT_TRUE(analyze_result.value()) << "分析文件返回false";

    // 创建日志识别器
    core::log_identifier::LogIdentifier log_identifier(config_, ast_analyzer);

    // 识别日志调用
    auto identify_result = log_identifier.identifyLogCalls();
    ASSERT_FALSE(identify_result.hasError()) << "识别日志调用失败: " << identify_result.errorMessage();
    ASSERT_TRUE(identify_result.value()) << "识别日志调用返回false";

    // 创建覆盖率计算器
    core::coverage::CoverageCalculator coverage_calculator(config_, ast_analyzer, log_identifier);

    // 计算覆盖率
    ASSERT_TRUE(coverage_calculator.calculate()) << "计算覆盖率失败";

    // 获取总体覆盖率统计信息
    const auto& overall_stats = coverage_calculator.getOverallCoverageStats();

    // 验证覆盖率计算是否进行
    EXPECT_GE(overall_stats.functionCoverage, 0.0);
    EXPECT_LE(overall_stats.functionCoverage, 1.0);
    EXPECT_GE(overall_stats.branchCoverage, 0.0);
    EXPECT_LE(overall_stats.branchCoverage, 1.0);
    EXPECT_GE(overall_stats.exceptionCoverage, 0.0);
    EXPECT_LE(overall_stats.exceptionCoverage, 1.0);
}

// 测试多文件覆盖率计算
TEST_F(CoverageWorkflowTest, MultiFileCoverageCalculation) {
    // 创建主文件
    std::string main_source = R"(
#include <QDebug>
#include "helper.h"

class MainProcessor {
public:
    void process() {
        qDebug() << "Main processing started";

        Helper helper;
        try {
            helper.doWork();
            qInfo() << "Main processing completed";
        } catch (const std::exception& e) {
            qCritical() << "Main processing failed:" << e.what();
        }
    }
};

int main() {
    MainProcessor processor;
    processor.process();
    return 0;
}
    )";

    // 创建辅助文件
    std::string helper_header = R"(
#pragma once

class Helper {
public:
    void doWork();
};
    )";

    std::string helper_source = R"(
#include "helper.h"
#include <QDebug>
#include <stdexcept>

void Helper::doWork() {
    qDebug() << "Helper work started";

    try {
        for (int i = 0; i < 3; ++i) {
            qDebug() << "Helper iteration" << i;
            if (i == 2) {
                throw std::runtime_error("Helper error");
            }
        }
    } catch (const std::exception& e) {
        qWarning() << "Helper caught error:" << e.what();
        throw;
    }
}
    )";

    // 创建文件
    std::string main_path = createTestSource("main_processor.cpp", main_source);
    std::string helper_header_path = createTestSource("helper.h", helper_header);
    std::string helper_source_path = createTestSource("helper.cpp", helper_source);

    // 初始化源文件管理器
    auto source_manager = TestUtils::createTestSourceManager(config_);

    // 收集源文件
    auto collect_result = source_manager->collectSourceFiles();
    ASSERT_FALSE(collect_result.hasError()) << "收集源文件失败: " << collect_result.errorMessage();
    ASSERT_TRUE(collect_result.value()) << "未能有效收集源文件";

    // 创建AST分析器
    core::ast_analyzer::ASTAnalyzer ast_analyzer(config_, *source_manager);

    // 分析所有文件
    auto analyze_result = ast_analyzer.analyzeAll();
    ASSERT_FALSE(analyze_result.hasError()) << "分析所有文件失败: " << analyze_result.errorMessage();
    ASSERT_TRUE(analyze_result.value()) << "分析文件返回false";

    // 创建日志识别器
    core::log_identifier::LogIdentifier log_identifier(config_, ast_analyzer);

    // 识别日志调用
    auto identify_result = log_identifier.identifyLogCalls();
    ASSERT_FALSE(identify_result.hasError()) << "识别日志调用失败: " << identify_result.errorMessage();
    ASSERT_TRUE(identify_result.value()) << "识别日志调用返回false";

    // 创建覆盖率计算器
    core::coverage::CoverageCalculator coverage_calculator(config_, ast_analyzer, log_identifier);

    // 计算覆盖率
    ASSERT_TRUE(coverage_calculator.calculate()) << "计算覆盖率失败";

    // 获取总体覆盖率统计信息
    const auto& overall_stats = coverage_calculator.getOverallCoverageStats();

    // 验证覆盖率计算是否进行
    EXPECT_GE(overall_stats.functionCoverage, 0.0);
    EXPECT_LE(overall_stats.functionCoverage, 1.0);
    EXPECT_GE(overall_stats.branchCoverage, 0.0);
    EXPECT_LE(overall_stats.branchCoverage, 1.0);
    EXPECT_GE(overall_stats.exceptionCoverage, 0.0);
    EXPECT_LE(overall_stats.exceptionCoverage, 1.0);
}

}  // namespace test
}  // namespace dlogcover
