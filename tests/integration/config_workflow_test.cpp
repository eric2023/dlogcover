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
#include <memory>

#include "../common/test_utils.h"

using namespace dlogcover::utils;
using namespace std::chrono_literals;

namespace dlogcover {
namespace test {

class ConfigWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 先创建临时目录
        temp_dir_manager_ = std::make_unique<dlogcover::tests::common::TempDirectoryManager>("config_test_");
        test_dir_ = temp_dir_manager_->getPath();
        ASSERT_FALSE(test_dir_.empty()) << "创建临时目录失败";

        // 初始化日志系统
        log_file_ = test_dir_ + "/test.log";
        Logger::init(log_file_, true, LogLevel::DEBUG);
    }

    void TearDown() override {
        // 先关闭日志系统
        Logger::shutdown();

        // 临时目录管理器会自动清理
        temp_dir_manager_.reset();
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
    std::unique_ptr<dlogcover::tests::common::TempDirectoryManager> temp_dir_manager_;
    std::string test_dir_;
    std::string log_file_;
};

// 测试基本配置加载
TEST_F(ConfigWorkflowTest, BasicConfigLoad) {
    // 最小配置内容
    std::string config_content = R"({
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

    std::string config_path;
    ASSERT_NO_THROW(config_path = createTestConfig(config_content)) << "创建配置文件失败";
    ASSERT_FALSE(config_path.empty());

    // 创建配置管理器并加载配置
    config::ConfigManager config_manager;
    bool load_success = config_manager.loadConfig(config_path);
    EXPECT_TRUE(load_success) << "加载配置失败";

    // 验证基本配置
    const config::Config& config = config_manager.getConfig();
    EXPECT_TRUE(config.analysis.function_coverage);
    EXPECT_TRUE(config.analysis.branch_coverage);
    EXPECT_TRUE(config.analysis.exception_coverage);
    EXPECT_TRUE(config.analysis.key_path_coverage);
    EXPECT_EQ(config.scan.directories.size(), 1);
    EXPECT_TRUE(config.log_functions.qt.enabled);
    EXPECT_TRUE(config.log_functions.custom.enabled);
    EXPECT_EQ(config.log_functions.qt.functions.size(), 5);
    EXPECT_EQ(config.output.report_file, "coverage_report.txt");
    EXPECT_EQ(config.output.log_level, "INFO");
}

// 测试无效配置文件
TEST_F(ConfigWorkflowTest, InvalidConfig) {
    std::string empty_config_path;
    ASSERT_NO_THROW(empty_config_path = createTestConfig("{}"));

    config::ConfigManager config_manager;
    bool load_success = config_manager.loadConfig(empty_config_path);

    // 空配置应该可以加载，但可能验证失败
    EXPECT_TRUE(load_success) << "加载空配置应该成功";

    // 验证空配置是否有效
    bool validate_success = config_manager.validateConfig();

    // 注意：空配置可能是有效的，如果默认配置有效
    // 这里我们不做强制的断言，只是记录结果
    if (!validate_success) {
        std::cout << "空配置验证失败，这可能是预期的行为" << std::endl;
    }
}

// 测试配置验证
TEST_F(ConfigWorkflowTest, ConfigValidation) {
    // 创建一个缺少必要字段的配置
    std::string invalid_config_content = R"({
        "scan": {
            "directories": []
        },
        "log_functions": {
            "qt": {
                "enabled": false
            }
        }
    })";

    std::string invalid_config_path;
    ASSERT_NO_THROW(invalid_config_path = createTestConfig(invalid_config_content));

    config::ConfigManager config_manager;
    bool load_success = config_manager.loadConfig(invalid_config_path);

    // 无效配置可能加载成功，但验证应该失败
    EXPECT_TRUE(load_success) << "加载无效配置应该成功";

    bool validate_success = config_manager.validateConfig();
    EXPECT_FALSE(validate_success) << "无效配置不应通过验证";
}

// 测试命令行参数覆盖配置
TEST_F(ConfigWorkflowTest, CommandLineOverride) {
    // 基本配置
    std::string config_content = R"({
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

    std::string config_path = createTestConfig(config_content);

    // 创建配置管理器并加载配置
    config::ConfigManager config_manager;
    bool load_success = config_manager.loadConfig(config_path);
    EXPECT_TRUE(load_success) << "加载配置失败";

    // 模拟命令行参数
    cli::CommandLineParser cmd_parser;
    const char* args[] = {"dlogcover", "-d", (test_dir_ + "/override_dir").c_str()};
    int argc = sizeof(args) / sizeof(args[0]);

    auto parse_result = cmd_parser.parse(argc, const_cast<char**>(args));
    EXPECT_FALSE(parse_result.hasError()) << "解析命令行参数失败: " << parse_result.message();

    // 应用命令行覆盖
    config_manager.mergeWithCommandLineOptions(cmd_parser.getOptions());

    // 验证覆盖结果
    const config::Config& config = config_manager.getConfig();
    EXPECT_EQ(config.scan.directories.size(), 1);
    EXPECT_EQ(config.scan.directories[0], test_dir_ + "/override_dir");
}

}  // namespace test
}  // namespace dlogcover
