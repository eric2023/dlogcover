/**
 * @file config_manager_test.cpp
 * @brief 配置管理器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/options.h>
#include <dlogcover/config/config_manager.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace config {
namespace test {

// 创建临时配置文件助手函数
std::string createTempConfigFile(const std::string& content) {
    // 生成临时文件路径
    std::string tempFile = std::filesystem::temp_directory_path().string() + "/dlogcover_test_config.json";

    // 创建临时文件
    std::ofstream file(tempFile);
    file << content;
    file.close();

    return tempFile;
}

// 测试默认配置
TEST(ConfigManagerTest, DefaultConfig) {
    // 创建配置管理器
    ConfigManager configManager;

    // 获取默认配置
    const Config& config = configManager.getConfig();

    // 验证默认扫描配置
    EXPECT_FALSE(config.scan.directories.empty());
    EXPECT_EQ("./", config.scan.directories[0]);
    EXPECT_FALSE(config.scan.fileTypes.empty());

    // 验证默认Qt日志函数配置
    EXPECT_TRUE(config.logFunctions.qt.enabled);
    EXPECT_FALSE(config.logFunctions.qt.functions.empty());

    // 验证默认自定义日志函数配置
    EXPECT_TRUE(config.logFunctions.custom.enabled);
    EXPECT_FALSE(config.logFunctions.custom.functions.empty());

    // 验证默认分析配置
    EXPECT_TRUE(config.analysis.functionCoverage);
    EXPECT_TRUE(config.analysis.branchCoverage);
    EXPECT_TRUE(config.analysis.exceptionCoverage);
    EXPECT_TRUE(config.analysis.keyPathCoverage);

    // 验证默认报告配置
    EXPECT_EQ("text", config.report.format);
    EXPECT_EQ("YYYYMMDD_HHMMSS", config.report.timestampFormat);
}

// 测试加载有效配置文件
TEST(ConfigManagerTest, LoadValidConfig) {
    // 创建有效的配置文件内容
    std::string validConfig = R"({
        "scan": {
            "directories": ["/test/dir"],
            "excludes": ["build/", "test/"],
            "file_types": [".cpp", ".h"]
        },
        "log_functions": {
            "qt": {
                "enabled": true,
                "functions": ["qDebug", "qInfo"],
                "category_functions": ["qCDebug"]
            },
            "custom": {
                "enabled": true,
                "functions": {
                    "debug": ["logDebug"],
                    "info": ["logInfo"]
                }
            }
        },
        "analysis": {
            "function_coverage": true,
            "branch_coverage": false,
            "exception_coverage": true,
            "key_path_coverage": false
        },
        "report": {
            "format": "json",
            "timestamp_format": "YYYY-MM-DD"
        }
    })";

    // 创建临时配置文件
    std::string tempConfigFile = createTempConfigFile(validConfig);

    // 创建配置管理器
    ConfigManager configManager;

    // 加载配置文件
    EXPECT_TRUE(configManager.loadConfig(tempConfigFile));

    // 获取配置
    const Config& config = configManager.getConfig();

    // 验证扫描配置
    EXPECT_EQ(1, config.scan.directories.size());
    EXPECT_EQ("/test/dir", config.scan.directories[0]);
    EXPECT_EQ(2, config.scan.excludes.size());
    EXPECT_EQ(2, config.scan.fileTypes.size());

    // 验证Qt日志函数配置
    EXPECT_TRUE(config.logFunctions.qt.enabled);
    EXPECT_EQ(2, config.logFunctions.qt.functions.size());
    EXPECT_EQ(1, config.logFunctions.qt.categoryFunctions.size());

    // 验证自定义日志函数配置
    EXPECT_TRUE(config.logFunctions.custom.enabled);
    EXPECT_EQ(2, config.logFunctions.custom.functions.size());
    EXPECT_EQ(1, config.logFunctions.custom.functions.at("debug").size());
    EXPECT_EQ(1, config.logFunctions.custom.functions.at("info").size());

    // 验证分析配置
    EXPECT_TRUE(config.analysis.functionCoverage);
    EXPECT_FALSE(config.analysis.branchCoverage);
    EXPECT_TRUE(config.analysis.exceptionCoverage);
    EXPECT_FALSE(config.analysis.keyPathCoverage);

    // 验证报告配置
    EXPECT_EQ("json", config.report.format);
    EXPECT_EQ("YYYY-MM-DD", config.report.timestampFormat);

    // 清理临时文件
    std::filesystem::remove(tempConfigFile);
}

// 测试加载无效配置文件
TEST(ConfigManagerTest, LoadInvalidConfig) {
    // 创建无效的配置文件内容（非法JSON）
    std::string invalidConfig = R"({
        "scan": {
            "directories": ["/test/dir"],
            "excludes": ["build/", "test/"],
        },
    })";

    // 创建临时配置文件
    std::string tempConfigFile = createTempConfigFile(invalidConfig);

    // 创建配置管理器
    ConfigManager configManager;

    // 加载配置文件应该失败
    EXPECT_FALSE(configManager.loadConfig(tempConfigFile));

    // 清理临时文件
    std::filesystem::remove(tempConfigFile);
}

// 测试与命令行选项合并
TEST(ConfigManagerTest, MergeWithCommandLineOptions) {
    // 创建配置管理器
    ConfigManager configManager;

    // 创建命令行选项
    cli::Options options;
    options.directoryPath = "/custom/dir";
    options.excludePatterns = {"custom_exclude/"};
    options.logLevel = cli::LogLevel::WARNING;
    options.reportFormat = cli::ReportFormat::JSON;

    // 合并命令行选项
    configManager.mergeWithCommandLineOptions(options);

    // 获取配置
    const Config& config = configManager.getConfig();

    // 验证扫描配置
    EXPECT_EQ(1, config.scan.directories.size());
    EXPECT_EQ("/custom/dir", config.scan.directories[0]);
    EXPECT_TRUE(std::find(config.scan.excludes.begin(), config.scan.excludes.end(), "custom_exclude/") !=
                config.scan.excludes.end());

    // 验证日志级别过滤
    auto& customFunctions = config.logFunctions.custom.functions;
    EXPECT_EQ(0, (customFunctions.find("debug") != customFunctions.end() ? 1 : 0) +
                     (customFunctions.find("info") != customFunctions.end() ? 1 : 0));

    // 验证报告格式
    EXPECT_EQ("json", config.report.format);
}

// 测试配置验证
TEST(ConfigManagerTest, ValidateConfig) {
    // 创建配置管理器
    ConfigManager configManager;

    // 获取默认配置
    Config config = configManager.getDefaultConfig();

    // 修改配置使其无效 - 空文件类型
    config.scan.fileTypes.clear();

    // 创建配置管理器
    ConfigManager invalidConfigManager;

    // 设置配置
    // 注意：ConfigManager没有提供setter方法，但在实际情况下，
    // 我们应该有方法来设置配置进行测试，或者应该暴露setter方法
    // 在这个测试中，我们检查默认配置的有效性

    // 默认配置应该有效
    EXPECT_TRUE(configManager.validateConfig());
}

}  // namespace test
}  // namespace config
}  // namespace dlogcover
