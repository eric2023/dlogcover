/**
 * @file options_extended_test.cpp
 * @brief 命令行选项扩展测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/options.h>
#include <dlogcover/cli/config_constants.h>
#include <dlogcover/cli/error_types.h>

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace dlogcover::cli;

namespace dlogcover {
namespace test {

class OptionsExtendedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        testDir_ = "/tmp/dlogcover_options_test";
        if (std::filesystem::exists(testDir_)) {
            std::filesystem::remove_all(testDir_);
        }
        std::filesystem::create_directories(testDir_);
        
        // 创建测试文件
        testFile_ = testDir_ + "/test_file.txt";
        std::ofstream file(testFile_);
        file << "test content";
        file.close();
        
        options_ = std::make_unique<Options>();
    }

    void TearDown() override {
        if (std::filesystem::exists(testDir_)) {
            std::filesystem::remove_all(testDir_);
        }
    }

    void createTestConfigFile(const std::string& content) {
        configFile_ = testDir_ + "/test_config.json";
        std::ofstream file(configFile_);
        file << content;
        file.close();
    }

    std::string testDir_;
    std::string testFile_;
    std::string configFile_;
    std::unique_ptr<Options> options_;
};

// 测试路径验证
TEST_F(OptionsExtendedTest, ValidateExistingDirectory) {
    options_->directory = testDir_;
    
    auto result = options_->validate();
    EXPECT_FALSE(result.hasError());
}

TEST_F(OptionsExtendedTest, ValidateNonexistentDirectory) {
    options_->directory = testDir_ + "/nonexistent_subdir";
    
    auto result = options_->validate();
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::DirectoryNotFound);
    EXPECT_TRUE(result.message().find("不存在") != std::string::npos);
}

TEST_F(OptionsExtendedTest, ValidateFileAsDirectory) {
    options_->directory = testFile_;  // 使用文件路径作为目录
    
    auto result = options_->validate();
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::DirectoryNotFound);
}

TEST_F(OptionsExtendedTest, ValidateEmptyDirectory) {
    options_->directory = "";  // 空路径应该允许（使用默认值）
    
    auto result = options_->validate();
    EXPECT_FALSE(result.hasError());
}

TEST_F(OptionsExtendedTest, ValidateExistingConfigFile) {
    createTestConfigFile(R"({"test": "config"})");
    options_->configPath = configFile_;
    
    auto result = options_->validate();
    EXPECT_FALSE(result.hasError());
}

TEST_F(OptionsExtendedTest, ValidateNonexistentConfigFile) {
    options_->configPath = testDir_ + "/nonexistent_config.json";
    
    auto result = options_->validate();
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::FileNotFound);
}

TEST_F(OptionsExtendedTest, ValidateOutputFileWithValidParentDir) {
    options_->output_file = testDir_ + "/output.txt";
    
    auto result = options_->validate();
    EXPECT_FALSE(result.hasError());
}

TEST_F(OptionsExtendedTest, ValidateOutputFileWithInvalidParentDir) {
    options_->output_file = testDir_ + "/nonexistent_subdir/output.txt";
    
    auto result = options_->validate();
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::OutputDirectoryNotFound);
}

TEST_F(OptionsExtendedTest, ValidateEmptyExcludePattern) {
    options_->excludePatterns = {"valid_pattern", "", "another_valid"};
    
    auto result = options_->validate();
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::InvalidExcludePattern);
}

TEST_F(OptionsExtendedTest, ValidateValidExcludePatterns) {
    options_->excludePatterns = {"*.tmp", "*/build/*", "test_*"};
    
    auto result = options_->validate();
    EXPECT_FALSE(result.hasError());
}

// 测试 JSON 序列化
TEST_F(OptionsExtendedTest, JsonSerializationBasic) {
    options_->directory = "/test/dir";
    options_->output_file = "/test/output.txt";
    options_->configPath = "/test/config.json";
    options_->excludePatterns = {"*.tmp", "*/build/*"};
    options_->logLevel = LogLevel::INFO;
    options_->reportFormat = ReportFormat::JSON;
    
    std::string json = options_->toJson();
    EXPECT_FALSE(json.empty());
    
    // 验证 JSON 格式正确
    auto parsed = nlohmann::json::parse(json);
    EXPECT_EQ(parsed["directory"], "/test/dir");
    EXPECT_EQ(parsed["output"], "/test/output.txt");
    EXPECT_EQ(parsed["config"], "/test/config.json");
    EXPECT_EQ(parsed["exclude"].size(), 2);
    EXPECT_EQ(parsed["log_level"], "info");
    EXPECT_EQ(parsed["report_format"], "json");
    EXPECT_TRUE(parsed.count("version") > 0);
}

TEST_F(OptionsExtendedTest, JsonSerializationEmptyExcludePatterns) {
    options_->excludePatterns.clear();
    
    std::string json = options_->toJson();
    auto parsed = nlohmann::json::parse(json);
    
    EXPECT_TRUE(parsed["exclude"].is_array());
    EXPECT_EQ(parsed["exclude"].size(), 0);
}

// 测试 JSON 反序列化
TEST_F(OptionsExtendedTest, JsonDeserializationValid) {
    std::string validJson = R"({
        "version": "1.0",
        "directory": "/test/dir",
        "output": "/test/output.txt",
        "config": "/test/config.json",
        "exclude": ["*.tmp", "*/build/*"],
        "log_level": "DEBUG",
        "report_format": "json"
    })";
    
    auto result = options_->fromJson(validJson);
    EXPECT_FALSE(result.hasError());
    
    EXPECT_EQ(options_->directory, "/test/dir");
    EXPECT_EQ(options_->output_file, "/test/output.txt");
    EXPECT_EQ(options_->configPath, "/test/config.json");
    EXPECT_EQ(options_->excludePatterns.size(), 2);
    EXPECT_EQ(options_->logLevel, LogLevel::DEBUG);
    EXPECT_EQ(options_->reportFormat, ReportFormat::JSON);
}

TEST_F(OptionsExtendedTest, JsonDeserializationMissingVersion) {
    std::string invalidJson = R"({
        "directory": "/test/dir",
        "output": "/test/output.txt"
    })";
    
    auto result = options_->fromJson(invalidJson);
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::InvalidVersion);
}

TEST_F(OptionsExtendedTest, JsonDeserializationInvalidVersion) {
    std::string invalidJson = R"({
        "version": "2.0",
        "directory": "/test/dir"
    })";
    
    auto result = options_->fromJson(invalidJson);
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::InvalidVersion);
}

TEST_F(OptionsExtendedTest, JsonDeserializationMissingDirectory) {
    std::string invalidJson = R"({
        "version": "1.0",
        "output": "/test/output.txt"
    })";
    
    auto result = options_->fromJson(invalidJson);
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::MissingField);
    EXPECT_TRUE(result.message().find("directory") != std::string::npos);
}

TEST_F(OptionsExtendedTest, JsonDeserializationWithDefaults) {
    std::string minimalJson = R"({
        "version": "1.0",
        "directory": "/test/dir"
    })";
    
    auto result = options_->fromJson(minimalJson);
    EXPECT_FALSE(result.hasError());
    
    EXPECT_EQ(options_->directory, "/test/dir");
    // 验证使用了默认值
    EXPECT_EQ(options_->output_file, std::string(config::cli::DEFAULT_OUTPUT));
    EXPECT_EQ(options_->configPath, std::string(config::cli::DEFAULT_CONFIG));
    EXPECT_TRUE(options_->excludePatterns.empty());
}

TEST_F(OptionsExtendedTest, JsonDeserializationInvalidLogLevel) {
    std::string invalidJson = R"({
        "version": "1.0",
        "directory": "/test/dir",
        "log_level": "INVALID_LEVEL"
    })";
    
    auto result = options_->fromJson(invalidJson);
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::InvalidLogLevel);
}

TEST_F(OptionsExtendedTest, JsonDeserializationInvalidReportFormat) {
    std::string invalidJson = R"({
        "version": "1.0",
        "directory": "/test/dir",
        "report_format": "INVALID_FORMAT"
    })";
    
    auto result = options_->fromJson(invalidJson);
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::InvalidReportFormat);
}

TEST_F(OptionsExtendedTest, JsonDeserializationMalformedJson) {
    std::string malformedJson = R"({
        "version": "1.0",
        "directory": "/test/dir"
        // 缺少闭合括号和逗号
    )";
    
    auto result = options_->fromJson(malformedJson);
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::ParseError);
}

// 测试选项验证和操作
TEST_F(OptionsExtendedTest, IsValidFunction) {
    // 测试有效选项
    options_->directory = testDir_;
    EXPECT_TRUE(options_->isValid());
    
    // 测试无效选项
    options_->directory = testDir_ + "/nonexistent_path";
    EXPECT_FALSE(options_->isValid());
}

TEST_F(OptionsExtendedTest, ResetFunction) {
    // 设置非默认值
    options_->directory = "/custom/dir";
    options_->output_file = "/custom/output.txt";
    options_->excludePatterns = {"custom_pattern"};
    options_->logLevel = LogLevel::ERROR;
    options_->reportFormat = ReportFormat::JSON;
    
    // 重置
    options_->reset();
    
    // 验证恢复到默认值
    EXPECT_EQ(options_->directory, std::string(config::cli::DEFAULT_DIRECTORY));
    EXPECT_EQ(options_->output_file, std::string(config::cli::DEFAULT_OUTPUT));
    EXPECT_EQ(options_->configPath, std::string(config::cli::DEFAULT_CONFIG));
    EXPECT_TRUE(options_->excludePatterns.empty());
    EXPECT_EQ(options_->logLevel, LogLevel::ALL);
    EXPECT_EQ(options_->reportFormat, ReportFormat::TEXT);
}

TEST_F(OptionsExtendedTest, ToStringFunction) {
    options_->directory = "/test/dir";
    options_->output_file = "/test/output.txt";
    options_->excludePatterns = {"*.tmp", "*/build/*"};
    
    std::string str = options_->toString();
    EXPECT_FALSE(str.empty());
    EXPECT_TRUE(str.find("/test/dir") != std::string::npos);
    EXPECT_TRUE(str.find("/test/output.txt") != std::string::npos);
    EXPECT_TRUE(str.find("*.tmp") != std::string::npos);
    EXPECT_TRUE(str.find("*/build/*") != std::string::npos);
}

// 测试比较操作符
TEST_F(OptionsExtendedTest, EqualityOperator) {
    Options options1, options2;
    
    // 默认情况下应该相等
    EXPECT_TRUE(options1 == options2);
    EXPECT_FALSE(options1 != options2);
    
    // 修改一个选项
    options1.directory = "/different/dir";
    EXPECT_FALSE(options1 == options2);
    EXPECT_TRUE(options1 != options2);
    
    // 恢复相等
    options2.directory = "/different/dir";
    EXPECT_TRUE(options1 == options2);
    EXPECT_FALSE(options1 != options2);
}

// 测试枚举转换函数
TEST_F(OptionsExtendedTest, LogLevelConversion) {
    EXPECT_EQ(cli::toString(LogLevel::DEBUG), "debug");
    EXPECT_EQ(cli::toString(LogLevel::INFO), "info");
    EXPECT_EQ(cli::toString(LogLevel::WARNING), "warning");
    EXPECT_EQ(cli::toString(LogLevel::ERROR), "error");
    EXPECT_EQ(cli::toString(LogLevel::ALL), "all");
    
    EXPECT_EQ(cli::parseLogLevel("DEBUG"), LogLevel::DEBUG);
    EXPECT_EQ(cli::parseLogLevel("INFO"), LogLevel::INFO);
    EXPECT_EQ(cli::parseLogLevel("WARNING"), LogLevel::WARNING);
    EXPECT_EQ(cli::parseLogLevel("ERROR"), LogLevel::ERROR);
    EXPECT_EQ(cli::parseLogLevel("ALL"), LogLevel::ALL);
}

TEST_F(OptionsExtendedTest, ReportFormatConversion) {
    EXPECT_EQ(cli::toString(ReportFormat::TEXT), "text");
    EXPECT_EQ(cli::toString(ReportFormat::JSON), "json");
    
    EXPECT_EQ(cli::parseReportFormat("text"), ReportFormat::TEXT);
    EXPECT_EQ(cli::parseReportFormat("json"), ReportFormat::JSON);
    EXPECT_EQ(cli::parseReportFormat("TEXT"), ReportFormat::TEXT);
    EXPECT_EQ(cli::parseReportFormat("JSON"), ReportFormat::JSON);
}

TEST_F(OptionsExtendedTest, InvalidReportFormatConversion) {
    EXPECT_THROW(cli::parseReportFormat("invalid"), std::invalid_argument);
    EXPECT_THROW(cli::parseReportFormat("xml"), std::invalid_argument);
}

// 测试 JSON 序列化和反序列化的往返
TEST_F(OptionsExtendedTest, JsonRoundTrip) {
    // 设置原始选项
    options_->directory = "/test/dir";
    options_->output_file = "/test/output.txt";
    options_->configPath = "/test/config.json";
    options_->excludePatterns = {"*.tmp", "*/build/*", "test_*"};
    options_->logLevel = LogLevel::WARNING;
    options_->reportFormat = ReportFormat::JSON;
    
    // 序列化
    std::string json = options_->toJson();
    
    // 创建新的选项对象并反序列化
    Options newOptions;
    auto result = newOptions.fromJson(json);
    
    EXPECT_FALSE(result.hasError());
    EXPECT_TRUE(*options_ == newOptions);
}

}  // namespace test
}  // namespace dlogcover 