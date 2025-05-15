/**
 * @file basic_integration_test.cpp
 * @brief 基本功能集成测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/cli/command_line_parser.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>
#include <fstream>
#include <filesystem>

namespace dlogcover {
namespace test {

// 测试基本工作流程
TEST(BasicIntegrationTest, BasicWorkflow) {
    // 创建临时配置文件
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
    
    std::string configPath = utils::FileUtils::createTempFile(configContent);
    ASSERT_FALSE(configPath.empty());
    
    // 初始化日志
    utils::Logger::init("test.log", true, utils::LogLevel::INFO);
    
    // 创建配置管理器
    config::ConfigManager configManager;
    EXPECT_TRUE(configManager.loadConfig(configPath));
    
    // 验证配置内容
    const config::Config& config = configManager.getConfig();
    EXPECT_TRUE(config.analysis.functionCoverage);
    EXPECT_TRUE(config.analysis.branchCoverage);
    EXPECT_TRUE(config.analysis.exceptionCoverage);
    EXPECT_TRUE(config.analysis.keyPathCoverage);
    
    // 清理
    std::filesystem::remove(configPath);
    utils::Logger::shutdown();
}

// 添加更多集成测试...

} // namespace test
} // namespace dlogcover 