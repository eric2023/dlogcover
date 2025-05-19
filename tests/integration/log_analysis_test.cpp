/**
 * @file log_analysis_test.cpp
 * @brief 日志分析集成测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/command_line_parser.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
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

class LogAnalysisTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_dir_ = TestUtils::createTestTempDir("log_test_");
        ASSERT_FALSE(test_dir_.empty());

        // 初始化日志系统
        log_file_ = test_dir_ + "/test.log";
        utils::Logger::init(log_file_, true, utils::LogLevel::INFO);

        // 创建测试源文件目录
        source_dir_ = test_dir_ + "/src";
        std::filesystem::create_directories(source_dir_);

        // 初始化配置
        config_ = TestUtils::createTestConfig(test_dir_);

        // 初始化源代码管理器
        source_manager_ = TestUtils::createTestSourceManager(config_);
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
    std::unique_ptr<source_manager::SourceManager> source_manager_;
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

    // 配置Qt日志函数（已经在config_中配置）
    // 创建分析器
    core::ast_analyzer::ASTAnalyzer ast_analyzer(config_, *source_manager_);
    core::log_identifier::LogIdentifier identifier(config_, ast_analyzer);

    // 分析源文件
    EXPECT_NO_THROW(ast_analyzer.analyze(source_path));
    EXPECT_NO_THROW(identifier.identifyLogCalls());

    // 验证结果
    const auto& log_calls = identifier.getAllLogCalls();
    EXPECT_FALSE(log_calls.empty());

    if (!log_calls.empty() && log_calls.find(source_path) != log_calls.end()) {
        const auto& file_logs = log_calls.at(source_path);
        EXPECT_GE(file_logs.size(), 4);  // 至少应该识别到4个Qt日志函数调用

        std::set<std::string> log_functions;
        for (const auto& log_call : file_logs) {
            log_functions.insert(log_call.functionName);
        }

        EXPECT_TRUE(log_functions.find("qDebug") != log_functions.end());
        EXPECT_TRUE(log_functions.find("qInfo") != log_functions.end());
        EXPECT_TRUE(log_functions.find("qWarning") != log_functions.end());
        EXPECT_TRUE(log_functions.find("qCritical") != log_functions.end());
    }
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

    // 创建分析器
    core::ast_analyzer::ASTAnalyzer ast_analyzer(config_, *source_manager_);
    core::log_identifier::LogIdentifier identifier(config_, ast_analyzer);

    // 分析源文件
    EXPECT_NO_THROW(ast_analyzer.analyze(source_path));
    EXPECT_NO_THROW(identifier.identifyLogCalls());

    // 验证结果
    const auto& log_calls = identifier.getAllLogCalls();
    EXPECT_FALSE(log_calls.empty());

    if (!log_calls.empty() && log_calls.find(source_path) != log_calls.end()) {
        const auto& file_logs = log_calls.at(source_path);
        EXPECT_GE(file_logs.size(), 3);  // 至少应该识别到3个自定义日志函数调用

        std::set<std::string> log_functions;
        for (const auto& log_call : file_logs) {
            log_functions.insert(log_call.functionName);
        }

        EXPECT_TRUE(log_functions.find("LOG_DEBUG") != log_functions.end());
        EXPECT_TRUE(log_functions.find("LOG_INFO") != log_functions.end());
        EXPECT_TRUE(log_functions.find("LOG_ERROR") != log_functions.end());
    }
}

// 测试条件分支中的日志
TEST_F(LogAnalysisTest, ConditionalLogAnalysis) {
    // 创建测试源文件
    std::string source = R"(
        #include <QDebug>
        #include <stdexcept>

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

    // 创建分析器
    core::ast_analyzer::ASTAnalyzer ast_analyzer(config_, *source_manager_);
    core::log_identifier::LogIdentifier identifier(config_, ast_analyzer);

    // 分析源文件
    EXPECT_NO_THROW(ast_analyzer.analyze(source_path));
    EXPECT_NO_THROW(identifier.identifyLogCalls());

    // 验证结果 - 检查AST节点是否正确构建
    const auto& ast_nodes = ast_analyzer.getAllASTNodeInfo();
    EXPECT_FALSE(ast_nodes.empty());

    if (!ast_nodes.empty() && ast_nodes.find(source_path) != ast_nodes.end()) {
        const auto& node_info = ast_nodes.at(source_path).get();

        // 验证是否有日志记录
        EXPECT_TRUE(node_info->hasLogging);

        // 检查是否存在if语句、for语句和try语句节点
        bool has_if = false;
        bool has_for = false;
        bool has_try = false;

        // 递归检查节点
        std::function<void(const core::ast_analyzer::ASTNodeInfo*)> checkNode;
        checkNode = [&](const core::ast_analyzer::ASTNodeInfo* node) {
            if (node->type == core::ast_analyzer::NodeType::IF_STMT) {
                has_if = true;
            } else if (node->type == core::ast_analyzer::NodeType::FOR_STMT) {
                has_for = true;
            } else if (node->type == core::ast_analyzer::NodeType::TRY_STMT) {
                has_try = true;
            }

            for (const auto& child : node->children) {
                checkNode(child.get());
            }
        };

        checkNode(node_info);

        EXPECT_TRUE(has_if);
        EXPECT_TRUE(has_for);
        EXPECT_TRUE(has_try);
    }

    // 检查日志函数调用
    const auto& log_calls = identifier.getAllLogCalls();
    EXPECT_FALSE(log_calls.empty());
    EXPECT_TRUE(log_calls.find(source_path) != log_calls.end());
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
                    }
                } catch (const std::exception& e) {
                    qCritical() << "Error:" << e.what();
                }
            }
        };
    )";

    std::string source_path = createTestSource("complex_scenario.cpp", source);

    // 创建分析器
    core::ast_analyzer::ASTAnalyzer ast_analyzer(config_, *source_manager_);
    core::log_identifier::LogIdentifier identifier(config_, ast_analyzer);

    // 分析源文件
    EXPECT_NO_THROW(ast_analyzer.analyze(source_path));
    EXPECT_NO_THROW(identifier.identifyLogCalls());

    // 验证结果
    const auto& log_calls = identifier.getAllLogCalls();
    EXPECT_FALSE(log_calls.empty());

    if (!log_calls.empty() && log_calls.find(source_path) != log_calls.end()) {
        const auto& file_logs = log_calls.at(source_path);

        std::set<std::string> log_functions;
        for (const auto& log_call : file_logs) {
            log_functions.insert(log_call.functionName);
        }

        EXPECT_TRUE(log_functions.find("qDebug") != log_functions.end());
        EXPECT_TRUE(log_functions.find("qInfo") != log_functions.end());
        EXPECT_TRUE(log_functions.find("qCritical") != log_functions.end());
    }

    // 检查AST节点
    const auto* source_node = ast_analyzer.getASTNodeInfo(source_path);
    EXPECT_TRUE(source_node != nullptr);

    if (source_node) {
        // 验证节点是否包含日志
        EXPECT_TRUE(source_node->hasLogging);

        // 检查是否有类和方法节点
        bool has_method = false;

        // 递归检查节点
        std::function<void(const core::ast_analyzer::ASTNodeInfo*)> checkNode;
        checkNode = [&](const core::ast_analyzer::ASTNodeInfo* node) {
            if (node->type == core::ast_analyzer::NodeType::METHOD) {
                has_method = true;
            }

            for (const auto& child : node->children) {
                checkNode(child.get());
            }
        };

        checkNode(source_node);

        EXPECT_TRUE(has_method);
    }
}

}  // namespace test
}  // namespace dlogcover
