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

#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace test {

class ConfigWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_dir_ = utils::FileUtils::createTempDir();
        ASSERT_FALSE(test_dir_.empty());

        // 初始化日志系统
        log_file_ = test_dir_ + "/test.log";
        utils::Logger::init(log_file_, true, utils::LogLevel::INFO);
    }

    void TearDown() override {
        // 关闭日志系统
        utils::Logger::shutdown();

        // 清理临时目录
        if (!test_dir_.empty()) {
            std::filesystem::remove_all(test_dir_);
        }
    }

    // 创建测试配置文件
    std::string createTestConfig(const std::string& content) {
        std::string config_path = test_dir_ + "/test_config.json";
        std::ofstream config_file(config_path);
        EXPECT_TRUE(config_file.is_open());
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
    std::string configContent = R"({
        "scan": {
            "directories": ["./"],
            "excludes": ["build/", "test/"],
            "file_types": [".cpp", ".cc", ".cxx", ".h", ".hpp"]
        },
        "log_functions": {
            "qt": {
                "enabled": true,
                "functions": ["qDebug", "qInfo", "qWarning", "qCritical", "qFatal"],
                "category_functions": ["qCDebug", "qCInfo", "qCWarning", "qCCritical"]
            },
            "custom": {
                "enabled": true,
                "functions": {
                    "debug": ["LOG_DEBUG"],
                    "info": ["LOG_INFO"],
                    "warning": ["LOG_WARNING"],
                    "critical": ["LOG_ERROR"]
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

    std::string configPath = createTestConfig(configContent);
    ASSERT_FALSE(configPath.empty());

    // 创建配置管理器并加载配置
    config::ConfigManager configManager;
    EXPECT_TRUE(configManager.loadConfig(configPath));

    // 验证配置内容
    const config::Config& config = configManager.getConfig();
    EXPECT_TRUE(config.analysis.functionCoverage);
    EXPECT_TRUE(config.analysis.branchCoverage);
    EXPECT_TRUE(config.analysis.exceptionCoverage);
    EXPECT_TRUE(config.analysis.keyPathCoverage);

    // 验证扫描配置
    EXPECT_EQ(config.scan.directories.size(), 1);
    EXPECT_EQ(config.scan.directories[0], "./");
    EXPECT_EQ(config.scan.excludes.size(), 2);
    EXPECT_EQ(config.scan.fileTypes.size(), 5);

    // 验证日志函数配置
    EXPECT_TRUE(config.logFunctions.qt.enabled);
    EXPECT_EQ(config.logFunctions.qt.functions.size(), 5);
    EXPECT_EQ(config.logFunctions.qt.categoryFunctions.size(), 4);
    EXPECT_TRUE(config.logFunctions.custom.enabled);
    EXPECT_EQ(config.logFunctions.custom.functions.size(), 4);
}

// 测试无效配置文件
TEST_F(ConfigWorkflowTest, InvalidConfig) {
    // 测试空配置文件
    std::string emptyConfigPath = createTestConfig("{}");
    config::ConfigManager configManager;
    EXPECT_FALSE(configManager.loadConfig(emptyConfigPath));

    // 测试格式错误的配置文件
    std::string invalidConfigPath = createTestConfig("{ invalid json }");
    EXPECT_FALSE(configManager.loadConfig(invalidConfigPath));

    // 测试缺少必要字段的配置文件
    std::string incompleteConfig = R"({
        "scan": {
            "directories": ["./"]
        }
    })";
    std::string incompleteConfigPath = createTestConfig(incompleteConfig);
    EXPECT_FALSE(configManager.loadConfig(incompleteConfigPath));
}

// 测试配置文件不存在的情况
TEST_F(ConfigWorkflowTest, NonExistentConfig) {
    config::ConfigManager configManager;
    EXPECT_FALSE(configManager.loadConfig("non_existent_config.json"));
}

// 测试配置文件权限问题
TEST_F(ConfigWorkflowTest, ConfigPermissions) {
    std::string configPath = createTestConfig("{}");
    // 修改文件权限为只读
    std::filesystem::permissions(configPath, std::filesystem::perms::owner_read | std::filesystem::perms::group_read |
                                                 std::filesystem::perms::others_read);

    config::ConfigManager configManager;
    // 尝试重写配置文件（应该失败）
    EXPECT_FALSE(configManager.saveConfig(configPath));
}

}  // namespace test
}  // namespace dlogcover
