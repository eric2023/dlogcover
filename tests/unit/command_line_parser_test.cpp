/**
 * @file command_line_parser_test.cpp
 * @brief 命令行解析器的单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/command_line_parser.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace dlogcover {
namespace cli {
namespace test {

// 测试帮助请求检测
TEST(CommandLineParserTest, HelpRequest) {
    // 准备
    CommandLineParser parser;

    // 重定向标准输出以捕获输出信息
    std::streambuf* oldCout = std::cout.rdbuf();
    std::ostringstream capturedOutput;
    std::cout.rdbuf(capturedOutput.rdbuf());

    // 执行
    int argc = 2;
    char* argv[] = {(char*)"dlogcover", (char*)"--help", nullptr};
    auto result = parser.parse(argc, argv);

    // 恢复标准输出
    std::cout.rdbuf(oldCout);

    // 验证
    EXPECT_FALSE(result.hasError());  // 应该没有错误
    EXPECT_TRUE(parser.isHelpOrVersionRequest());
    EXPECT_TRUE(capturedOutput.str().find("DLogCover") != std::string::npos);
    EXPECT_TRUE(capturedOutput.str().find("用法:") != std::string::npos);
}

// 测试版本请求检测
TEST(CommandLineParserTest, VersionRequest) {
    // 准备
    CommandLineParser parser;

    // 重定向标准输出以捕获输出信息
    std::streambuf* oldCout = std::cout.rdbuf();
    std::ostringstream capturedOutput;
    std::cout.rdbuf(capturedOutput.rdbuf());

    // 执行
    int argc = 2;
    char* argv[] = {(char*)"dlogcover", (char*)"--version", nullptr};
    auto result = parser.parse(argc, argv);

    // 恢复标准输出
    std::cout.rdbuf(oldCout);

    // 验证
    EXPECT_FALSE(result.hasError());  // 应该没有错误
    EXPECT_TRUE(parser.isHelpOrVersionRequest());
    EXPECT_TRUE(capturedOutput.str().find("DLogCover v") != std::string::npos);
}

// 测试短格式帮助请求
TEST(CommandLineParserTest, ShortHelpRequest) {
    // 准备
    CommandLineParser parser;

    // 执行
    int argc = 2;
    char* argv[] = {(char*)"dlogcover", (char*)"-h", nullptr};

    // 重定向标准输出以捕获输出信息
    std::streambuf* oldCout = std::cout.rdbuf();
    std::ostringstream capturedOutput;
    std::cout.rdbuf(capturedOutput.rdbuf());

    auto result = parser.parse(argc, argv);

    // 恢复标准输出
    std::cout.rdbuf(oldCout);

    // 验证
    EXPECT_FALSE(result.hasError());  // 应该没有错误
    EXPECT_TRUE(parser.isHelpOrVersionRequest());
}

// 测试短格式版本请求
TEST(CommandLineParserTest, ShortVersionRequest) {
    // 准备
    CommandLineParser parser;

    // 执行
    int argc = 2;
    char* argv[] = {(char*)"dlogcover", (char*)"-v", nullptr};

    // 重定向标准输出以捕获输出信息
    std::streambuf* oldCout = std::cout.rdbuf();
    std::ostringstream capturedOutput;
    std::cout.rdbuf(capturedOutput.rdbuf());

    auto result = parser.parse(argc, argv);

    // 恢复标准输出
    std::cout.rdbuf(oldCout);

    // 验证
    EXPECT_FALSE(result.hasError());  // 应该没有错误
    EXPECT_TRUE(parser.isHelpOrVersionRequest());
}

// 测试普通参数（非帮助或版本请求）
TEST(CommandLineParserTest, NormalArguments) {
    // 准备
    CommandLineParser parser;

    // 执行
    int argc = 3;
    char* argv[] = {(char*)"dlogcover", (char*)"-d", (char*)"./src", nullptr};
    auto result = parser.parse(argc, argv);

    // 验证
    EXPECT_FALSE(result.hasError());  // 应该没有错误
    EXPECT_FALSE(parser.isHelpOrVersionRequest());
    EXPECT_EQ("./src", parser.getOptions().directoryPath);
}

// 测试无参数情况
TEST(CommandLineParserTest, NoArguments) {
    // 准备
    CommandLineParser parser;

    // 执行
    int argc = 1;
    char* argv[] = {(char*)"dlogcover", nullptr};
    auto result = parser.parse(argc, argv);

    // 验证
    EXPECT_FALSE(result.hasError());  // 应该没有错误
    EXPECT_FALSE(parser.isHelpOrVersionRequest());
    EXPECT_EQ("./", parser.getOptions().directoryPath);  // 使用默认值
}

// 测试无效参数处理
TEST(CommandLineParserTest, InvalidArguments) {
    // 准备
    CommandLineParser parser;

    // 重定向标准错误输出
    std::streambuf* oldCerr = std::cerr.rdbuf();
    std::ostringstream capturedError;
    std::cerr.rdbuf(capturedError.rdbuf());

    // 执行
    int argc = 2;
    char* argv[] = {(char*)"dlogcover", (char*)"--invalid", nullptr};
    auto result = parser.parse(argc, argv);

    // 恢复标准错误输出
    std::cerr.rdbuf(oldCerr);

    // 验证
    EXPECT_TRUE(result.hasError());  // 应该有错误
    EXPECT_EQ(ConfigError::UnknownOption, result.error());
    EXPECT_FALSE(parser.isHelpOrVersionRequest());
    EXPECT_TRUE(result.message().find("未知选项") != std::string::npos);
}

// 测试缺少参数值的情况
TEST(CommandLineParserTest, MissingArgumentValue) {
    // 准备
    CommandLineParser parser;

    // 重定向标准错误输出
    std::streambuf* oldCerr = std::cerr.rdbuf();
    std::ostringstream capturedError;
    std::cerr.rdbuf(capturedError.rdbuf());

    // 执行 - 缺少目录参数值
    int argc = 2;
    char* argv[] = {(char*)"dlogcover", (char*)"-d", nullptr};
    auto result = parser.parse(argc, argv);

    // 恢复标准错误输出
    std::cerr.rdbuf(oldCerr);

    // 验证
    EXPECT_TRUE(result.hasError());  // 应该有错误
    EXPECT_EQ(ConfigError::MissingArgument, result.error());
    EXPECT_FALSE(parser.isHelpOrVersionRequest());
    EXPECT_TRUE(result.message().find("缺少") != std::string::npos);
}

// 测试日志级别解析
TEST(CommandLineParserTest, LogLevelParsing) {
    // 准备
    CommandLineParser parser;

    // 测试有效日志级别
    const std::vector<std::pair<std::string, LogLevel>> validLevels = {
        {"debug", LogLevel::DEBUG}, {"DEBUG", LogLevel::DEBUG},  // 测试大写
        {"info", LogLevel::INFO},   {"warning", LogLevel::WARNING}, {"critical", LogLevel::CRITICAL},
        {"fatal", LogLevel::FATAL}, {"all", LogLevel::ALL}};

    for (const auto& [levelStr, expectedLevel] : validLevels) {
        int argc = 3;
        char* argv[] = {(char*)"dlogcover", (char*)"-l", (char*)levelStr.c_str(), nullptr};
        auto result = parser.parse(argc, argv);

        EXPECT_FALSE(result.hasError()) << "Failed to parse log level: " << levelStr;
        EXPECT_EQ(expectedLevel, parser.getOptions().logLevel) << "Incorrect log level for input: " << levelStr;
    }

    // 测试无效日志级别（应该使用默认值ALL）
    {
        int argc = 3;
        char* argv[] = {(char*)"dlogcover", (char*)"-l", (char*)"invalid", nullptr};
        auto result = parser.parse(argc, argv);

        EXPECT_TRUE(result.hasError());
        EXPECT_EQ(ConfigError::InvalidLogLevel, result.error());
    }
}

// 测试报告格式解析
TEST(CommandLineParserTest, ReportFormatParsing) {
    // 准备
    CommandLineParser parser;

    // 测试有效格式
    const std::vector<std::pair<std::string, ReportFormat>> validFormats = {
        {"text", ReportFormat::TEXT},
        {"TEXT", ReportFormat::TEXT},  // 测试大写
        {"json", ReportFormat::JSON},
        {"JSON", ReportFormat::JSON}  // 测试大写
    };

    for (const auto& [formatStr, expectedFormat] : validFormats) {
        int argc = 3;
        char* argv[] = {(char*)"dlogcover", (char*)"-f", (char*)formatStr.c_str(), nullptr};
        auto result = parser.parse(argc, argv);

        EXPECT_FALSE(result.hasError()) << "Failed to parse format: " << formatStr;
        EXPECT_EQ(expectedFormat, parser.getOptions().reportFormat) << "Incorrect format for input: " << formatStr;
    }

    // 测试无效格式（应该返回错误）
    {
        int argc = 3;
        char* argv[] = {(char*)"dlogcover", (char*)"-f", (char*)"invalid", nullptr};
        auto result = parser.parse(argc, argv);

        EXPECT_TRUE(result.hasError());
        EXPECT_EQ(ConfigError::InvalidReportFormat, result.error());
    }
}

// 测试文件路径验证
class CommandLineParserFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录和文件
        testDir = std::filesystem::temp_directory_path() / "dlogcover_test";
        std::filesystem::create_directories(testDir);

        configFile = testDir / "config.json";
        std::ofstream(configFile).close();

        outputDir = testDir / "output";
        std::filesystem::create_directories(outputDir);
    }

    void TearDown() override {
        // 清理临时文件和目录
        std::filesystem::remove_all(testDir);
    }

    std::filesystem::path testDir;
    std::filesystem::path configFile;
    std::filesystem::path outputDir;
};

TEST_F(CommandLineParserFileTest, DirectoryValidation) {
    CommandLineParser parser;

    // 测试有效目录
    {
        int argc = 3;
        std::string dirPath = testDir.string();
        char* argv[] = {(char*)"dlogcover", (char*)"-d", (char*)dirPath.c_str(), nullptr};
        auto result = parser.parse(argc, argv);

        EXPECT_FALSE(result.hasError());
        EXPECT_EQ(dirPath, parser.getOptions().directoryPath);
    }

    // 测试不存在的目录
    {
        int argc = 3;
        std::string invalidDir = (testDir / "nonexistent").string();
        char* argv[] = {(char*)"dlogcover", (char*)"-d", (char*)invalidDir.c_str(), nullptr};

        // 重定向标准错误输出
        std::streambuf* oldCerr = std::cerr.rdbuf();
        std::ostringstream capturedError;
        std::cerr.rdbuf(capturedError.rdbuf());

        auto result = parser.parse(argc, argv);

        // 恢复标准错误输出
        std::cerr.rdbuf(oldCerr);

        EXPECT_TRUE(result.hasError());
        EXPECT_EQ(ConfigError::DirectoryNotFound, result.error());
        EXPECT_TRUE(result.message().find("目录不存在") != std::string::npos);
    }
}

TEST_F(CommandLineParserFileTest, ConfigFileValidation) {
    CommandLineParser parser;

    // 测试有效配置文件
    {
        int argc = 3;
        std::string configPath = configFile.string();
        char* argv[] = {(char*)"dlogcover", (char*)"-c", (char*)configPath.c_str(), nullptr};
        auto result = parser.parse(argc, argv);

        EXPECT_FALSE(result.hasError());
        EXPECT_EQ(configPath, parser.getOptions().configPath);
    }

    // 测试不存在的配置文件
    {
        int argc = 3;
        std::string invalidConfig = (testDir / "nonexistent.json").string();
        char* argv[] = {(char*)"dlogcover", (char*)"-c", (char*)invalidConfig.c_str(), nullptr};

        // 重定向标准错误输出
        std::streambuf* oldCerr = std::cerr.rdbuf();
        std::ostringstream capturedError;
        std::cerr.rdbuf(capturedError.rdbuf());

        auto result = parser.parse(argc, argv);

        // 恢复标准错误输出
        std::cerr.rdbuf(oldCerr);

        EXPECT_TRUE(result.hasError());
        EXPECT_EQ(ConfigError::FileNotFound, result.error());
        EXPECT_TRUE(result.message().find("文件不存在") != std::string::npos);
    }
}

// 测试参数组合
TEST_F(CommandLineParserFileTest, ParameterCombination) {
    CommandLineParser parser;

    // 测试多个有效参数组合
    {
        int argc = 9;
        std::string dirPath = testDir.string();
        std::string outputPath = (outputDir / "report.txt").string();
        std::string configPath = configFile.string();
        char* argv[] = {(char*)"dlogcover",
                        (char*)"-d",
                        (char*)dirPath.c_str(),
                        (char*)"-o",
                        (char*)outputPath.c_str(),
                        (char*)"-c",
                        (char*)configPath.c_str(),
                        (char*)"-l",
                        (char*)"debug",
                        nullptr};
        auto result = parser.parse(argc, argv);

        EXPECT_FALSE(result.hasError());
        EXPECT_EQ(dirPath, parser.getOptions().directoryPath);
        EXPECT_EQ(outputPath, parser.getOptions().outputPath);
        EXPECT_EQ(configPath, parser.getOptions().configPath);
        EXPECT_EQ(LogLevel::DEBUG, parser.getOptions().logLevel);
    }

    // 测试多个排除模式
    {
        int argc = 7;
        char* argv[] = {(char*)"dlogcover", (char*)"-e", (char*)"build/*", (char*)"-e",
                        (char*)"test/*",    (char*)"-e", (char*)"*.tmp",   nullptr};
        auto result = parser.parse(argc, argv);

        EXPECT_FALSE(result.hasError());
        EXPECT_EQ(3, parser.getOptions().excludePatterns.size());
        EXPECT_EQ("build/*", parser.getOptions().excludePatterns[0]);
        EXPECT_EQ("test/*", parser.getOptions().excludePatterns[1]);
        EXPECT_EQ("*.tmp", parser.getOptions().excludePatterns[2]);
    }
}

}  // namespace test
}  // namespace cli
}  // namespace dlogcover

// 添加一个独立的main函数用于覆盖率测试
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
