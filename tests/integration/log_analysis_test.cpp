/**
 * @file log_analysis_test.cpp
 * @brief 日志分析集成测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/command_line_parser.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace test {

class LogAnalysisTest : public ::testing::Test {
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

// 测试Qt日志函数识别
TEST_F(LogAnalysisTest, QtLogFunctionIdentification) {
    // 创建测试源文件
    std::string source = R"(
        #include <QDebug>

        void testQtLogs() {
            qDebug() << "Debug message";
            qInfo() << "Info message";
            qWarning() << "Warning message";
            qCritical() << "Critical message";

            // 测试分类日志
            qCDebug(MyCategory) << "Category debug";
            qCInfo(MyCategory) << "Category info";
        }
    )";

    std::string source_path = createTestSource("qt_logs.cpp", source);

    // 创建日志标识器
    core::LogIdentifier identifier;

    // 配置Qt日志函数
    std::vector<std::string> qt_functions = {"qDebug", "qInfo", "qWarning", "qCritical"};
    std::vector<std::string> qt_category_functions = {"qCDebug", "qCInfo"};

    identifier.addQtLogFunctions(qt_functions);
    identifier.addQtCategoryFunctions(qt_category_functions);

    // 分析源文件
    auto result = identifier.analyze(source_path);

    // 验证结果
    EXPECT_EQ(result.logCalls.size(), 6);
    EXPECT_TRUE(result.hasLogFunction("qDebug"));
    EXPECT_TRUE(result.hasLogFunction("qInfo"));
    EXPECT_TRUE(result.hasLogFunction("qWarning"));
    EXPECT_TRUE(result.hasLogFunction("qCritical"));
    EXPECT_TRUE(result.hasLogFunction("qCDebug"));
    EXPECT_TRUE(result.hasLogFunction("qCInfo"));
}

// 测试自定义日志函数识别
TEST_F(LogAnalysisTest, CustomLogFunctionIdentification) {
    // 创建测试源文件
    std::string source = R"(
        #include <iostream>

        #define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
        #define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl
        #define LOG_ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl

        void testCustomLogs() {
            LOG_DEBUG("Debug message");
            LOG_INFO("Info message");
            LOG_ERROR("Error message");

            if (true) {
                LOG_DEBUG("Conditional debug");
            }
        }
    )";

    std::string source_path = createTestSource("custom_logs.cpp", source);

    // 创建日志标识器
    core::LogIdentifier identifier;

    // 配置自定义日志函数
    std::unordered_map<std::string, std::vector<std::string>> custom_functions = {
        {"debug", {"LOG_DEBUG"}}, {"info", {"LOG_INFO"}}, {"error", {"LOG_ERROR"}}};

    identifier.addCustomLogFunctions(custom_functions);

    // 分析源文件
    auto result = identifier.analyze(source_path);

    // 验证结果
    EXPECT_EQ(result.logCalls.size(), 4);
    EXPECT_TRUE(result.hasLogFunction("LOG_DEBUG"));
    EXPECT_TRUE(result.hasLogFunction("LOG_INFO"));
    EXPECT_TRUE(result.hasLogFunction("LOG_ERROR"));
}

// 测试条件分支中的日志
TEST_F(LogAnalysisTest, ConditionalLogAnalysis) {
    // 创建测试源文件
    std::string source = R"(
        #include <QDebug>

        void testConditionalLogs(bool condition) {
            if (condition) {
                qDebug() << "Condition true";
            } else {
                qDebug() << "Condition false";
            }

            for (int i = 0; i < 3; ++i) {
                qInfo() << "Loop iteration" << i;
            }

            try {
                throw std::runtime_error("Error");
            } catch (const std::exception& e) {
                qCritical() << "Exception:" << e.what();
            }
        }
    )";

    std::string source_path = createTestSource("conditional_logs.cpp", source);

    // 创建AST分析器
    core::ASTAnalyzer analyzer;

    // 分析源文件
    auto result = analyzer.analyze(source_path);

    // 验证结果
    EXPECT_TRUE(result.hasBranchCoverage);
    EXPECT_TRUE(result.hasLoopCoverage);
    EXPECT_TRUE(result.hasExceptionCoverage);
}

// 测试复杂场景
TEST_F(LogAnalysisTest, ComplexScenario) {
    // 创建测试源文件
    std::string source = R"(
        #include <QDebug>
        #include <stdexcept>

        class TestClass {
        public:
            void complexMethod(int value) {
                try {
                    if (value < 0) {
                        qDebug() << "Negative value:" << value;
                        throw std::invalid_argument("Value cannot be negative");
                    }

                    for (int i = 0; i < value; ++i) {
                        if (i % 2 == 0) {
                            qInfo() << "Processing even number:" << i;
                        } else {
                            qDebug() << "Processing odd number:" << i;
                        }

                        try {
                            if (i == value - 1) {
                                throw std::runtime_error("Last iteration");
                            }
                        } catch (const std::exception& e) {
                            qWarning() << "Inner exception:" << e.what();
                        }
                    }
                } catch (const std::exception& e) {
                    qCritical() << "Outer exception:" << e.what();
                }
            }
        };
    )";

    std::string source_path = createTestSource("complex_scenario.cpp", source);

    // 创建分析器
    core::LogIdentifier identifier;
    core::ASTAnalyzer ast_analyzer;

    // 配置Qt日志函数
    std::vector<std::string> qt_functions = {"qDebug", "qInfo", "qWarning", "qCritical"};
    identifier.addQtLogFunctions(qt_functions);

    // 分析源文件
    auto log_result = identifier.analyze(source_path);
    auto ast_result = ast_analyzer.analyze(source_path);

    // 验证日志分析结果
    EXPECT_TRUE(log_result.hasLogFunction("qDebug"));
    EXPECT_TRUE(log_result.hasLogFunction("qInfo"));
    EXPECT_TRUE(log_result.hasLogFunction("qWarning"));
    EXPECT_TRUE(log_result.hasLogFunction("qCritical"));

    // 验证AST分析结果
    EXPECT_TRUE(ast_result.hasBranchCoverage);
    EXPECT_TRUE(ast_result.hasLoopCoverage);
    EXPECT_TRUE(ast_result.hasExceptionCoverage);
    EXPECT_TRUE(ast_result.hasNestedExceptionHandling);
}

}  // namespace test
}  // namespace dlogcover
