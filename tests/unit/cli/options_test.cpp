/**
 * @file options_test.cpp
 * @brief 命令行选项单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/options.h>
#include <dlogcover/cli/config_constants.h>
#include <dlogcover/cli/error_types.h>

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace cli {
namespace test {

class OptionsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试目录和文件
        std::filesystem::create_directories("test_data");
        std::filesystem::create_directories("test_data/output");
        std::ofstream("test_data/config.json") << "{}";
    }

    void TearDown() override {
        // 清理测试目录和文件
        std::filesystem::remove_all("test_data");
    }
};

// 测试默认构造函数
TEST_F(OptionsTest, DefaultConstructor) {
    Options options;
    EXPECT_EQ(options.directoryPath, std::string(config::cli::DEFAULT_DIRECTORY));
    EXPECT_EQ(options.outputPath, std::string(config::cli::DEFAULT_OUTPUT));
    EXPECT_EQ(options.configPath, std::string(config::cli::DEFAULT_CONFIG));
    EXPECT_TRUE(options.excludePatterns.empty());
    EXPECT_EQ(options.logLevel, LogLevel::ALL);
    EXPECT_EQ(options.reportFormat, ReportFormat::TEXT);
}

// 测试重置功能
TEST_F(OptionsTest, Reset) {
    Options options;
    options.directoryPath = "/custom/path";
    options.outputPath = "/custom/output.txt";
    options.configPath = "/custom/config.json";
    options.excludePatterns = {"pattern1", "pattern2"};
    options.logLevel = LogLevel::DEBUG;
    options.reportFormat = ReportFormat::JSON;

    options.reset();

    EXPECT_EQ(options.directoryPath, std::string(config::cli::DEFAULT_DIRECTORY));
    EXPECT_EQ(options.outputPath, std::string(config::cli::DEFAULT_OUTPUT));
    EXPECT_EQ(options.configPath, std::string(config::cli::DEFAULT_CONFIG));
    EXPECT_TRUE(options.excludePatterns.empty());
    EXPECT_EQ(options.logLevel, LogLevel::ALL);
    EXPECT_EQ(options.reportFormat, ReportFormat::TEXT);
}

// 测试验证功能
TEST_F(OptionsTest, Validate) {
    Options options;

    // 测试默认值
    EXPECT_FALSE(options.validate().hasError());

    // 测试无效目录
    options.directoryPath = "/nonexistent/directory";
    auto result = options.validate();
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::DirectoryNotFound);

    // 测试有效目录
    options.directoryPath = "test_data";
    EXPECT_FALSE(options.validate().hasError());

    // 测试无效配置文件
    options.configPath = "/nonexistent/config.json";
    result = options.validate();
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::FileNotFound);

    // 测试有效配置文件
    options.configPath = "test_data/config.json";
    EXPECT_FALSE(options.validate().hasError());

    // 测试无效输出目录
    options.outputPath = "/nonexistent/output/file.txt";
    result = options.validate();
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::OutputDirectoryNotFound);

    // 测试有效输出目录
    options.outputPath = "test_data/output/file.txt";
    EXPECT_FALSE(options.validate().hasError());

    // 测试空排除模式
    options.excludePatterns = {""};
    result = options.validate();
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::InvalidExcludePattern);

    // 测试有效排除模式
    options.excludePatterns = {"pattern1", "pattern2"};
    EXPECT_FALSE(options.validate().hasError());
}

// 测试字符串转换
TEST_F(OptionsTest, ToString) {
    Options options;
    options.directoryPath = "test_data";
    options.outputPath = "test_data/output/file.txt";
    options.configPath = "test_data/config.json";
    options.excludePatterns = {"pattern1", "pattern2"};
    options.logLevel = LogLevel::DEBUG;
    options.reportFormat = ReportFormat::JSON;

    std::string str = options.toString();
    EXPECT_TRUE(str.find("test_data") != std::string::npos);
    EXPECT_TRUE(str.find("pattern1") != std::string::npos);
    EXPECT_TRUE(str.find("pattern2") != std::string::npos);
    EXPECT_TRUE(str.find("debug") != std::string::npos);
    EXPECT_TRUE(str.find("json") != std::string::npos);
}

// 测试JSON序列化和反序列化
TEST_F(OptionsTest, JsonSerialization) {
    Options options1;
    options1.directoryPath = "test_data";
    options1.outputPath = "test_data/output/file.txt";
    options1.configPath = "test_data/config.json";
    options1.excludePatterns = {"pattern1", "pattern2"};
    options1.logLevel = LogLevel::DEBUG;
    options1.reportFormat = ReportFormat::JSON;

    // 序列化
    std::string json = options1.toJson();
    EXPECT_TRUE(json.find("test_data") != std::string::npos);
    EXPECT_TRUE(json.find("pattern1") != std::string::npos);
    EXPECT_TRUE(json.find("debug") != std::string::npos);
    EXPECT_TRUE(json.find("json") != std::string::npos);

    // 反序列化
    Options options2;
    auto result = options2.fromJson(json);
    EXPECT_FALSE(result.hasError());
    EXPECT_EQ(options1, options2);

    // 测试无效JSON
    result = options2.fromJson("invalid json");
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::ParseError);

    // 测试缺少版本
    result = options2.fromJson(R"({"directory": "test"})");
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::InvalidVersion);

    // 测试无效版本
    result = options2.fromJson(R"({"version": "0.0", "directory": "test"})");
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::InvalidVersion);

    // 测试缺少必需字段
    result = options2.fromJson(R"({"version": "1.0"})");
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::MissingField);

    // 测试无效日志级别
    result = options2.fromJson(R"(
        {
            "version": "1.0",
            "directory": "test",
            "log_level": "invalid"
        }
    )");
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::InvalidLogLevel);

    // 测试无效报告格式
    result = options2.fromJson(R"(
        {
            "version": "1.0",
            "directory": "test",
            "report_format": "invalid"
        }
    )");
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), ConfigError::InvalidReportFormat);
}

// 测试比较操作符
TEST_F(OptionsTest, ComparisonOperators) {
    Options options1;
    Options options2;

    // 默认值应该相等
    EXPECT_EQ(options1, options2);
    EXPECT_FALSE(options1 != options2);

    // 修改一个字段后应该不相等
    options2.directoryPath = "different";
    EXPECT_NE(options1, options2);
    EXPECT_FALSE(options1 == options2);

    // 恢复相同值后应该相等
    options2.directoryPath = options1.directoryPath;
    EXPECT_EQ(options1, options2);
}

// 测试日志级别转换
TEST_F(OptionsTest, LogLevelConversion) {
    // 测试所有有效值
    EXPECT_EQ(parseLogLevel("debug"), LogLevel::DEBUG);
    EXPECT_EQ(parseLogLevel("info"), LogLevel::INFO);
    EXPECT_EQ(parseLogLevel("warning"), LogLevel::WARNING);
    EXPECT_EQ(parseLogLevel("critical"), LogLevel::CRITICAL);
    EXPECT_EQ(parseLogLevel("fatal"), LogLevel::FATAL);
    EXPECT_EQ(parseLogLevel("all"), LogLevel::ALL);

    // 测试大小写不敏感
    EXPECT_EQ(parseLogLevel("DEBUG"), LogLevel::DEBUG);
    EXPECT_EQ(parseLogLevel("Info"), LogLevel::INFO);

    // 测试无效值
    EXPECT_THROW(parseLogLevel("invalid"), std::invalid_argument);
    EXPECT_THROW(parseLogLevel(""), std::invalid_argument);

    // 测试转换回字符串
    EXPECT_EQ(toString(LogLevel::DEBUG), config::log::DEBUG);
    EXPECT_EQ(toString(LogLevel::INFO), config::log::INFO);
    EXPECT_EQ(toString(LogLevel::WARNING), config::log::WARNING);
    EXPECT_EQ(toString(LogLevel::CRITICAL), config::log::CRITICAL);
    EXPECT_EQ(toString(LogLevel::FATAL), config::log::FATAL);
    EXPECT_EQ(toString(LogLevel::ALL), config::log::ALL);
}

// 测试报告格式转换
TEST_F(OptionsTest, ReportFormatConversion) {
    // 测试所有有效值
    EXPECT_EQ(parseReportFormat("text"), ReportFormat::TEXT);
    EXPECT_EQ(parseReportFormat("json"), ReportFormat::JSON);

    // 测试大小写不敏感
    EXPECT_EQ(parseReportFormat("TEXT"), ReportFormat::TEXT);
    EXPECT_EQ(parseReportFormat("Json"), ReportFormat::JSON);

    // 测试无效值
    EXPECT_THROW(parseReportFormat("invalid"), std::invalid_argument);
    EXPECT_THROW(parseReportFormat(""), std::invalid_argument);

    // 测试转换回字符串
    EXPECT_EQ(toString(ReportFormat::TEXT), config::report::TEXT);
    EXPECT_EQ(toString(ReportFormat::JSON), config::report::JSON);
}

}  // namespace test
}  // namespace cli
}  // namespace dlogcover
