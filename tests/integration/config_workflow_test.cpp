/**
 * @file config_workflow_test.cpp
 * @brief 配置工作流集成测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/command_line_parser.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>

#include "../common/test_utils.h"

using namespace dlogcover::utils;
using namespace std::chrono_literals;

namespace dlogcover {
namespace test {

class ConfigWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 先创建临时目录
        test_dir_ = TestUtils::createTestTempDir();
        ASSERT_FALSE(test_dir_.empty()) << "创建临时目录失败";

        // 初始化日志系统
        log_file_ = test_dir_ + "/test.log";
        Logger::init(log_file_, true, LogLevel::DEBUG);
    }

    void TearDown() override {
        // 先关闭日志系统
        Logger::shutdown();

        // 再清理临时目录
        if (!test_dir_.empty()) {
            EXPECT_TRUE(TestUtils::cleanupTestTempDir(test_dir_)) << "清理临时目录失败";
        }
    }

    // 创建测试配置文件
    std::string createTestConfig(const std::string& content) {
        std::string config_path = test_dir_ + "/test_config.json";
        std::ofstream config_file(config_path);
        if (!config_file.is_open()) {
            throw std::runtime_error("无法创建配置文件");
        }
        config_file << content;
        config_file.close();
        return config_path;
    }

protected:
    std::string test_dir_;
    std::string log_file_;
};

// 测试基本配置加载
TEST_F(ConfigWorkflowTest, BasicConfigLoad) {
    // 最小配置内容
    std::string configContent = R"({
        "scan": {
            "directories": ["./"],
            "file_types": [".cpp", ".h", ".cc", ".hpp"],
            "is_qt_project": true
        },
        "log_functions": {
            "qt": {
                "enabled": true,
                "functions": ["qDebug", "qInfo", "qWarning", "qCritical", "qFatal"]
            },
            "custom": {
                "enabled": true,
                "functions": {
                    "debug": ["LogDebug", "log_debug"],
                    "info": ["LogInfo", "log_info"],
                    "warning": ["LogWarning", "log_warning"],
                    "error": ["LogError", "log_error"]
                }
            }
        },
        "analysis": {
            "function_coverage": true,
            "branch_coverage": true,
            "exception_coverage": true,
            "key_path_coverage": true
        },
        "report": {
            "format": "text",
            "timestamp_format": "YYYYMMDD_HHMMSS"
        }
    })";

    std::string configPath;
    ASSERT_NO_THROW(configPath = createTestConfig(configContent)) << "创建配置文件失败";
    ASSERT_FALSE(configPath.empty());

    // 创建配置管理器并加载配置
    config::ConfigManager configManager;
    auto loadResult = configManager.loadConfig(configPath);
    EXPECT_FALSE(loadResult.hasError()) << "加载配置失败: " << loadResult.errorMessage();
    EXPECT_TRUE(loadResult.value()) << "加载配置返回false";

    // 验证基本配置
    const config::Config& config = configManager.getConfig();
    EXPECT_TRUE(config.analysis.functionCoverage);
    EXPECT_TRUE(config.analysis.branchCoverage);
    EXPECT_TRUE(config.analysis.exceptionCoverage);
    EXPECT_TRUE(config.analysis.keyPathCoverage);
    EXPECT_EQ(config.scan.directories.size(), 1);
    EXPECT_TRUE(config.scan.isQtProject);
    EXPECT_TRUE(config.logFunctions.qt.enabled);
    EXPECT_TRUE(config.logFunctions.custom.enabled);
    EXPECT_EQ(config.logFunctions.qt.functions.size(), 5);
    EXPECT_EQ(config.report.format, "text");
    EXPECT_EQ(config.report.timestampFormat, "YYYYMMDD_HHMMSS");
}

// 测试无效配置文件
TEST_F(ConfigWorkflowTest, InvalidConfig) {
    std::string emptyConfigPath;
    ASSERT_NO_THROW(emptyConfigPath = createTestConfig("{}"));

    config::ConfigManager configManager;
    auto loadResult = configManager.loadConfig(emptyConfigPath);

    // 空配置可能是错误或者返回false
    if (loadResult.hasError()) {
        EXPECT_TRUE(loadResult.hasError()) << "应该报告错误";
    } else {
        EXPECT_FALSE(loadResult.value()) << "不应该成功加载空配置";
    }
}

// 测试配置验证
TEST_F(ConfigWorkflowTest, ConfigValidation) {
    // 创建一个缺少必要字段的配置
    std::string invalidConfigContent = R"({
        "scan": {
            "directories": []
        },
        "log_functions": {
            "qt": {
                "enabled": false
            }
        }
    })";

    std::string invalidConfigPath;
    ASSERT_NO_THROW(invalidConfigPath = createTestConfig(invalidConfigContent));

    config::ConfigManager configManager;
    auto loadResult = configManager.loadConfig(invalidConfigPath);

    // 可能是加载错误或者加载成功但验证失败
    if (loadResult.hasError()) {
        EXPECT_TRUE(loadResult.hasError()) << "无效配置应该报告错误";
    } else {
        auto validateResult = configManager.validateConfig();
        EXPECT_FALSE(validateResult.value()) << "无效配置不应通过验证";
    }
}

// 测试命令行参数覆盖配置
TEST_F(ConfigWorkflowTest, CommandLineOverride) {
    // 基本配置
    std::string configContent = R"({
        "scan": {
            "directories": ["./default_dir"],
            "file_types": [".cpp", ".h"]
        },
        "log_functions": {
            "qt": {
                "enabled": false
            }
        },
        "analysis": {
            "function_coverage": false
        },
        "report": {
            "format": "text"
        }
    })";

    std::string configPath = createTestConfig(configContent);

    // 创建配置管理器并加载配置
    config::ConfigManager configManager;
    auto loadResult = configManager.loadConfig(configPath);
    EXPECT_FALSE(loadResult.hasError()) << "加载配置失败: " << loadResult.errorMessage();

    // 模拟命令行参数
    cli::CommandLineParser cmdParser;
    std::vector<const char*> args = {"dlogcover",    "-d",       (test_dir_ + "/override_dir").c_str(),
                                     "--qt-logging", "true",     "--function-coverage",
                                     "true",         "--format", "json"};

    auto parseResult = cmdParser.parse(args.size(), args.data());
    EXPECT_FALSE(parseResult.hasError()) << "解析命令行参数失败: " << parseResult.errorMessage();
    EXPECT_TRUE(parseResult.value()) << "解析命令行参数返回false";

    // 应用命令行覆盖
    auto applyResult = configManager.applyCommandLineOptions(cmdParser.getOptions());
    EXPECT_FALSE(applyResult.hasError()) << "应用命令行选项失败: " << applyResult.errorMessage();
    EXPECT_TRUE(applyResult.value()) << "应用命令行选项返回false";

    // 验证覆盖结果
    const config::Config& config = configManager.getConfig();
    EXPECT_EQ(config.scan.directories.size(), 1);
    EXPECT_EQ(config.scan.directories[0], test_dir_ + "/override_dir");
    EXPECT_TRUE(config.logFunctions.qt.enabled);
    EXPECT_TRUE(config.analysis.functionCoverage);
    EXPECT_EQ(config.report.format, "json");
}

}  // namespace test
}  // namespace dlogcover
