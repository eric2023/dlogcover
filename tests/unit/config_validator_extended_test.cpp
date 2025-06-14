/**
 * @file config_validator_extended_test.cpp
 * @brief ConfigValidator扩展测试 - 提高覆盖率
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/cli/config_validator.h>
#include <dlogcover/utils/log_utils.h>
#include "../common/test_utils.h"

#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace dlogcover {
namespace cli {
namespace test {

class ConfigValidatorExtendedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化日志系统
        utils::Logger::init("", false, utils::LogLevel::DEBUG);
        
        // 创建临时目录管理器
        tempDirManager_ = std::make_unique<tests::common::TempDirectoryManager>();
        testDir_ = tempDirManager_->getPath().string();
        
        validator_ = std::make_unique<ConfigValidator>();
    }

    void TearDown() override {
        validator_.reset();
        tempDirManager_.reset();
        utils::Logger::shutdown();
    }

    void createConfigFile(const std::string& filename, const std::string& content) {
        std::string filePath = testDir_ + "/" + filename;
        std::ofstream file(filePath);
        ASSERT_TRUE(file.is_open()) << "无法创建测试配置文件: " << filePath;
        file << content;
        file.close();
    }

    std::string getConfigPath(const std::string& filename) {
        return testDir_ + "/" + filename;
    }

    std::unique_ptr<tests::common::TempDirectoryManager> tempDirManager_;
    std::string testDir_;
    std::unique_ptr<ConfigValidator> validator_;
};

// 测试版本验证功能（通过配置文件间接测试）
TEST_F(ConfigValidatorExtendedTest, VersionValidation) {
    // 测试有效版本
    std::string validVersionConfig = R"({
        "version": "1.0",
        "directory": "/test/project"
    })";
    createConfigFile("valid_version.json", validVersionConfig);
    EXPECT_TRUE(validator_->validateConfig(getConfigPath("valid_version.json")));
    EXPECT_EQ(validator_->getConfigVersion(), "1.0");
    
    // 测试无效版本
    std::string invalidVersionConfig = R"({
        "version": "2.0",
        "directory": "/test/project"
    })";
    createConfigFile("invalid_version.json", invalidVersionConfig);
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_version.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidVersion);
    EXPECT_TRUE(validator_->getError().find("2.0") != std::string::npos);
    
    // 测试空版本
    std::string emptyVersionConfig = R"({
        "version": "",
        "directory": "/test/project"
    })";
    createConfigFile("empty_version.json", emptyVersionConfig);
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("empty_version.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidVersion);
}

// 测试嵌套格式配置验证
TEST_F(ConfigValidatorExtendedTest, NestedFormatValidation) {
    // 测试有效的嵌套格式配置
    std::string validNestedConfig = R"({
        "version": "1.0",
        "project": {
            "directory": "/test/project"
        },
        "output": {
            "report_file": "/test/report.txt",
            "log_file": "/test/log.txt",
            "log_level": "debug"
        },
        "scan": {
            "exclude_patterns": ["*.tmp", "build/*"]
        }
    })";
    
    createConfigFile("nested_config.json", validNestedConfig);
    EXPECT_TRUE(validator_->validateConfig(getConfigPath("nested_config.json")));
}

// 测试简化格式配置验证
TEST_F(ConfigValidatorExtendedTest, SimplifiedFormatValidation) {
    // 测试有效的简化格式配置
    std::string validSimpleConfig = R"({
        "version": "1.0",
        "directory": "/test/project",
        "output": "/test/report.txt",
        "log_path": "/test/log.txt",
        "log_level": "info",
        "report_format": "json",
        "exclude": ["*.tmp", "build/*"]
    })";
    
    createConfigFile("simple_config.json", validSimpleConfig);
    EXPECT_TRUE(validator_->validateConfig(getConfigPath("simple_config.json")));
}

// 测试配置文件错误处理
TEST_F(ConfigValidatorExtendedTest, ConfigFileErrorHandling) {
    // 测试文件不存在
    EXPECT_FALSE(validator_->validateConfig("/nonexistent/config.json"));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::FileNotFound);
    
    // 测试无效JSON格式
    createConfigFile("invalid.json", "{ invalid json }");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::ParseError);
    
    // 测试缺少必需字段
    createConfigFile("missing_field.json", R"({"version": "1.0"})");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("missing_field.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::MissingField);
}

// 测试类型验证错误
TEST_F(ConfigValidatorExtendedTest, TypeValidationErrors) {
    // 测试版本字段类型错误
    createConfigFile("invalid_version_type.json", R"({
        "version": 1.0,
        "directory": "/test"
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_version_type.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidType);
    
    // 测试目录字段类型错误（嵌套格式）
    createConfigFile("invalid_nested_dir_type.json", R"({
        "version": "1.0",
        "project": {
            "directory": 123
        }
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_nested_dir_type.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidType);
    
    // 测试目录字段类型错误（简化格式）
    createConfigFile("invalid_simple_dir_type.json", R"({
        "version": "1.0",
        "directory": 123
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_simple_dir_type.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidType);
}

// 测试输出配置验证
TEST_F(ConfigValidatorExtendedTest, OutputConfigValidation) {
    // 测试嵌套格式输出配置类型错误
    createConfigFile("invalid_output_nested.json", R"({
        "version": "1.0",
        "directory": "/test",
        "output": {
            "report_file": 123
        }
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_output_nested.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidType);
    
    // 测试输出配置无效类型
    createConfigFile("invalid_output_type.json", R"({
        "version": "1.0",
        "directory": "/test",
        "output": 123
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_output_type.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidType);
    
    // 测试简化格式输出字段类型错误
    createConfigFile("invalid_simple_output.json", R"({
        "version": "1.0",
        "directory": "/test",
        "output": 123
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_simple_output.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidType);
}

// 测试日志级别验证
TEST_F(ConfigValidatorExtendedTest, LogLevelValidation) {
    // 测试嵌套格式无效日志级别
    createConfigFile("invalid_nested_log_level.json", R"({
        "version": "1.0",
        "directory": "/test",
        "output": {
            "log_level": "invalid_level"
        }
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_nested_log_level.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidLogLevel);
    
    // 测试简化格式无效日志级别
    createConfigFile("invalid_simple_log_level.json", R"({
        "version": "1.0",
        "directory": "/test",
        "log_level": "invalid_level"
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_simple_log_level.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidLogLevel);
    
    // 测试日志级别字段类型错误
    createConfigFile("invalid_log_level_type.json", R"({
        "version": "1.0",
        "directory": "/test",
        "log_level": 123
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_log_level_type.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidType);
}

// 测试报告格式验证
TEST_F(ConfigValidatorExtendedTest, ReportFormatValidation) {
    // 测试无效报告格式
    createConfigFile("invalid_report_format.json", R"({
        "version": "1.0",
        "directory": "/test",
        "report_format": "invalid_format"
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_report_format.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidReportFormat);
    
    // 测试报告格式字段类型错误
    createConfigFile("invalid_report_format_type.json", R"({
        "version": "1.0",
        "directory": "/test",
        "report_format": 123
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_report_format_type.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidType);
}

// 测试扫描配置验证
TEST_F(ConfigValidatorExtendedTest, ScanConfigValidation) {
    // 测试嵌套格式扫描配置类型错误
    createConfigFile("invalid_scan_type.json", R"({
        "version": "1.0",
        "directory": "/test",
        "scan": "invalid_type"
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_scan_type.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidType);
    
    // 测试排除模式数组类型错误
    createConfigFile("invalid_exclude_array.json", R"({
        "version": "1.0",
        "directory": "/test",
        "scan": {
            "exclude_patterns": "not_an_array"
        }
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_exclude_array.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidType);
    
    // 测试排除模式项类型错误
    createConfigFile("invalid_exclude_item.json", R"({
        "version": "1.0",
        "directory": "/test",
        "scan": {
            "exclude_patterns": ["valid", 123]
        }
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_exclude_item.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidExcludePattern);
    
    // 测试简化格式排除模式类型错误
    createConfigFile("invalid_simple_exclude.json", R"({
        "version": "1.0",
        "directory": "/test",
        "exclude": "not_an_array"
    })");
    EXPECT_FALSE(validator_->validateConfig(getConfigPath("invalid_simple_exclude.json")));
    EXPECT_EQ(validator_->getErrorCode(), ConfigError::InvalidType);
}

// 测试从配置文件加载选项
TEST_F(ConfigValidatorExtendedTest, LoadFromConfigFile) {
    // 测试嵌套格式加载
    std::string nestedConfig = R"({
        "version": "1.0",
        "project": {
            "directory": "/nested/project"
        },
        "output": {
            "report_file": "/nested/report.txt",
            "log_file": "/nested/log.txt",
            "log_level": "warning"
        },
        "report_format": "json",
        "scan": {
            "exclude_patterns": ["*.tmp", "build/*"]
        }
    })";
    
    createConfigFile("nested_load.json", nestedConfig);
    
    Options options;
    EXPECT_TRUE(validator_->loadFromConfig(getConfigPath("nested_load.json"), options));
    
    EXPECT_EQ(options.directory, "/nested/project");
    EXPECT_EQ(options.output_file, "/nested/report.txt");
    EXPECT_EQ(options.log_file, "/nested/log.txt");
    EXPECT_EQ(options.logLevel, LogLevel::WARNING);
    EXPECT_EQ(options.reportFormat, ReportFormat::JSON);
    EXPECT_EQ(options.excludePatterns.size(), 2);
    EXPECT_EQ(options.excludePatterns[0], "*.tmp");
    EXPECT_EQ(options.excludePatterns[1], "build/*");
    
    // 测试简化格式加载
    std::string simpleConfig = R"({
        "version": "1.0",
        "directory": "/simple/project",
        "output": "/simple/report.txt",
        "log_path": "/simple/log.txt",
        "log_level": "critical",
        "report_format": "text",
        "exclude": ["*.bak"]
    })";
    
    createConfigFile("simple_load.json", simpleConfig);
    
    Options options2;
    EXPECT_TRUE(validator_->loadFromConfig(getConfigPath("simple_load.json"), options2));
    
    EXPECT_EQ(options2.directory, "/simple/project");
    EXPECT_EQ(options2.output_file, "/simple/report.txt");
    EXPECT_EQ(options2.log_file, "/simple/log.txt");
    EXPECT_EQ(options2.logLevel, LogLevel::CRITICAL);
    EXPECT_EQ(options2.reportFormat, ReportFormat::TEXT);
    EXPECT_EQ(options2.excludePatterns.size(), 1);
    EXPECT_EQ(options2.excludePatterns[0], "*.bak");
}

// 测试从环境变量加载选项
TEST_F(ConfigValidatorExtendedTest, LoadFromEnvironment) {
    // 设置环境变量
    setenv("DLOGCOVER_DIRECTORY", "/env/project", 1);
    setenv("DLOGCOVER_OUTPUT", "/env/report.txt", 1);
    setenv("DLOGCOVER_CONFIG", "/env/config.json", 1);
    setenv("DLOGCOVER_LOG_PATH", "/env/log.txt", 1);
    setenv("DLOGCOVER_LOG_LEVEL", "fatal", 1);
    setenv("DLOGCOVER_REPORT_FORMAT", "json", 1);
    setenv("DLOGCOVER_EXCLUDE", "*.tmp,build/*,test/*", 1);
    
    Options options;
    EXPECT_TRUE(validator_->loadFromEnvironment(options));
    
    EXPECT_EQ(options.directory, "/env/project");
    EXPECT_EQ(options.output_file, "/env/report.txt");
    EXPECT_EQ(options.configPath, "/env/config.json");
    EXPECT_EQ(options.log_file, "/env/log.txt");
    EXPECT_EQ(options.logLevel, LogLevel::FATAL);
    EXPECT_EQ(options.reportFormat, ReportFormat::JSON);
    EXPECT_EQ(options.excludePatterns.size(), 3);
    EXPECT_EQ(options.excludePatterns[0], "*.tmp");
    EXPECT_EQ(options.excludePatterns[1], "build/*");
    EXPECT_EQ(options.excludePatterns[2], "test/*");
    
    // 清理环境变量
    unsetenv("DLOGCOVER_DIRECTORY");
    unsetenv("DLOGCOVER_OUTPUT");
    unsetenv("DLOGCOVER_CONFIG");
    unsetenv("DLOGCOVER_LOG_PATH");
    unsetenv("DLOGCOVER_LOG_LEVEL");
    unsetenv("DLOGCOVER_REPORT_FORMAT");
    unsetenv("DLOGCOVER_EXCLUDE");
}

// 测试环境变量错误处理
TEST_F(ConfigValidatorExtendedTest, EnvironmentErrorHandling) {
    // 测试无效日志级别
    setenv("DLOGCOVER_LOG_LEVEL", "invalid_level", 1);
    
    Options options;
    EXPECT_TRUE(validator_->loadFromEnvironment(options)); // 应该成功，但忽略无效值
    
    // 测试无效报告格式
    setenv("DLOGCOVER_REPORT_FORMAT", "invalid_format", 1);
    EXPECT_TRUE(validator_->loadFromEnvironment(options)); // 应该成功，但忽略无效值
    
    // 清理环境变量
    unsetenv("DLOGCOVER_LOG_LEVEL");
    unsetenv("DLOGCOVER_REPORT_FORMAT");
}

// 测试所有日志级别解析
TEST_F(ConfigValidatorExtendedTest, AllLogLevelParsing) {
    std::vector<std::pair<std::string, LogLevel>> testCases = {
        {"debug", LogLevel::DEBUG},
        {"DEBUG", LogLevel::DEBUG},
        {"info", LogLevel::INFO},
        {"INFO", LogLevel::INFO},
        {"warning", LogLevel::WARNING},
        {"WARNING", LogLevel::WARNING},
        {"critical", LogLevel::CRITICAL},
        {"CRITICAL", LogLevel::CRITICAL},
        {"fatal", LogLevel::FATAL},
        {"FATAL", LogLevel::FATAL},
        {"all", LogLevel::ALL},
        {"ALL", LogLevel::ALL}
    };
    
    for (const auto& [levelStr, expectedLevel] : testCases) {
        std::string config = R"({
            "version": "1.0",
            "directory": "/test",
            "log_level": ")" + levelStr + R"("
        })";
        
        createConfigFile("level_test_" + levelStr + ".json", config);
        EXPECT_TRUE(validator_->validateConfig(getConfigPath("level_test_" + levelStr + ".json")))
            << "Failed to validate log level: " << levelStr;
    }
}

// 测试所有报告格式解析
TEST_F(ConfigValidatorExtendedTest, AllReportFormatParsing) {
    std::vector<std::pair<std::string, ReportFormat>> testCases = {
        {"text", ReportFormat::TEXT},
        {"TEXT", ReportFormat::TEXT},
        {"json", ReportFormat::JSON},
        {"JSON", ReportFormat::JSON}
    };
    
    for (const auto& [formatStr, expectedFormat] : testCases) {
        std::string config = R"({
            "version": "1.0",
            "directory": "/test",
            "report_format": ")" + formatStr + R"("
        })";
        
        createConfigFile("format_test_" + formatStr + ".json", config);
        EXPECT_TRUE(validator_->validateConfig(getConfigPath("format_test_" + formatStr + ".json")))
            << "Failed to validate report format: " << formatStr;
    }
}

}  // namespace test
}  // namespace cli
}  // namespace dlogcover