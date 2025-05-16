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
        "scan": {"directories": ["./"]},
        "log_functions": {"qt": {"enabled": true}},
        "analysis": {"function_coverage": true},
        "report": {"format": "text"}
    })";

    std::string configPath;
    ASSERT_NO_THROW(configPath = createTestConfig(configContent)) << "创建配置文件失败";
    ASSERT_FALSE(configPath.empty());

    // 创建配置管理器并加载配置
    config::ConfigManager configManager;
    EXPECT_TRUE(configManager.loadConfig(configPath)) << "加载配置失败";

    // 验证基本配置
    const config::Config& config = configManager.getConfig();
    EXPECT_TRUE(config.analysis.functionCoverage);
    EXPECT_EQ(config.scan.directories.size(), 1);
}

// 测试无效配置文件
TEST_F(ConfigWorkflowTest, InvalidConfig) {
    std::string emptyConfigPath;
    ASSERT_NO_THROW(emptyConfigPath = createTestConfig("{}"));

    config::ConfigManager configManager;
    EXPECT_FALSE(configManager.loadConfig(emptyConfigPath)) << "不应该成功加载空配置";
}

}  // namespace test
}  // namespace dlogcover
