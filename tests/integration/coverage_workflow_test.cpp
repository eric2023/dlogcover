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
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace test {

class CoverageWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_dir_ = utils::FileUtils::createTempDir();
        ASSERT_FALSE(test_dir_.empty());

        // 初始化日志系统
        log_file_ = test_dir_ + "/test.log";
        utils::Logger::init(log_file_, true, utils::LogLevel::INFO);

        // 创建测试源文件目录
        source_dir_ = test_dir_ + "/src";
        std::filesystem::create_directories(source_dir_);
    }

    void TearDown() override {
        // 关闭日志系统
        utils::Logger::shutdown();

        // 清理临时目录
        if (!test_dir_.empty()) {
            std::filesystem::remove_all(test_dir_);
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
};

// 测试基本覆盖率计算
TEST_F(CoverageWorkflowTest, BasicCoverageCalculation) {
    // 创建测试源文件
    std::string source = R"(
        #include <QDebug>

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
    )";

    std::string source_path = createTestSource("calculator.cpp", source);

    // 创建分析器
    core::LogIdentifier identifier;
    core::ASTAnalyzer ast_analyzer;
    core::CoverageCalculator calculator;

    // 配置Qt日志函数
    std::vector<std::string> qt_functions = {"qDebug", "qCritical"};
    identifier.addQtLogFunctions(qt_functions);

    // 分析源文件
    auto log_result = identifier.analyze(source_path);
    auto ast_result = ast_analyzer.analyze(source_path);

    // 计算覆盖率
    auto coverage = calculator.calculate(log_result, ast_result);

    // 验证覆盖率结果
    EXPECT_GT(coverage.functionCoverage, 80.0);   // 函数覆盖率应大于80%
    EXPECT_GT(coverage.branchCoverage, 80.0);     // 分支覆盖率应大于80%
    EXPECT_GT(coverage.exceptionCoverage, 90.0);  // 异常覆盖率应大于90%
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
    )";

    std::string source_path = createTestSource("data_processor.cpp", source);

    // 创建分析器
    core::LogIdentifier identifier;
    core::ASTAnalyzer ast_analyzer;
    core::CoverageCalculator calculator;

    // 配置Qt日志函数
    std::vector<std::string> qt_functions = {"qDebug", "qInfo", "qWarning", "qCritical"};
    identifier.addQtLogFunctions(qt_functions);

    // 分析源文件
    auto log_result = identifier.analyze(source_path);
    auto ast_result = ast_analyzer.analyze(source_path);

    // 计算覆盖率
    auto coverage = calculator.calculate(log_result, ast_result);

    // 验证覆盖率结果
    EXPECT_GT(coverage.functionCoverage, 90.0);   // 函数覆盖率应大于90%
    EXPECT_GT(coverage.branchCoverage, 85.0);     // 分支覆盖率应大于85%
    EXPECT_GT(coverage.exceptionCoverage, 90.0);  // 异常覆盖率应大于90%
    EXPECT_GT(coverage.keyPathCoverage, 85.0);    // 关键路径覆盖率应大于85%
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

    // 创建分析器
    core::LogIdentifier identifier;
    core::ASTAnalyzer ast_analyzer;
    core::CoverageCalculator calculator;

    // 配置Qt日志函数
    std::vector<std::string> qt_functions = {"qDebug", "qInfo", "qWarning", "qCritical"};
    identifier.addQtLogFunctions(qt_functions);

    // 分析所有源文件
    std::vector<std::string> source_files = {main_path, helper_source_path};

    core::LogIdentifier::Result combined_log_result;
    core::ASTAnalyzer::Result combined_ast_result;

    for (const auto& file : source_files) {
        auto log_result = identifier.analyze(file);
        auto ast_result = ast_analyzer.analyze(file);

        // 合并结果
        combined_log_result.merge(log_result);
        combined_ast_result.merge(ast_result);
    }

    // 计算总体覆盖率
    auto coverage = calculator.calculate(combined_log_result, combined_ast_result);

    // 验证覆盖率结果
    EXPECT_GT(coverage.functionCoverage, 85.0);   // 函数覆盖率应大于85%
    EXPECT_GT(coverage.branchCoverage, 85.0);     // 分支覆盖率应大于85%
    EXPECT_GT(coverage.exceptionCoverage, 90.0);  // 异常覆盖率应大于90%
    EXPECT_GT(coverage.keyPathCoverage, 85.0);    // 关键路径覆盖率应大于85%
}

}  // namespace test
}  // namespace dlogcover
