/**
 * @file command_line_parser_test.cpp
 * @brief 命令行解析器的单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/command_line_parser.h>

#include <gtest/gtest.h>

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
    bool result = parser.parse(argc, argv);

    // 恢复标准输出
    std::cout.rdbuf(oldCout);

    // 验证
    EXPECT_FALSE(result);  // 因为显示帮助后应该返回false
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
    bool result = parser.parse(argc, argv);

    // 恢复标准输出
    std::cout.rdbuf(oldCout);

    // 验证
    EXPECT_FALSE(result);  // 因为显示版本后应该返回false
    EXPECT_TRUE(parser.isHelpOrVersionRequest());
    EXPECT_TRUE(capturedOutput.str().find("DLogCover 版本") != std::string::npos);
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

    bool result = parser.parse(argc, argv);

    // 恢复标准输出
    std::cout.rdbuf(oldCout);

    // 验证
    EXPECT_FALSE(result);
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

    bool result = parser.parse(argc, argv);

    // 恢复标准输出
    std::cout.rdbuf(oldCout);

    // 验证
    EXPECT_FALSE(result);
    EXPECT_TRUE(parser.isHelpOrVersionRequest());
}

// 测试普通参数（非帮助或版本请求）
TEST(CommandLineParserTest, NormalArguments) {
    // 准备
    CommandLineParser parser;

    // 执行
    int argc = 3;
    char* argv[] = {(char*)"dlogcover", (char*)"-d", (char*)"./src", nullptr};
    bool result = parser.parse(argc, argv);

    // 验证
    EXPECT_TRUE(result);  // 应该返回true表示成功解析
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
    bool result = parser.parse(argc, argv);

    // 验证
    EXPECT_TRUE(result);  // 应该返回true表示使用默认值
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
    bool result = parser.parse(argc, argv);

    // 恢复标准错误输出
    std::cerr.rdbuf(oldCerr);

    // 验证
    EXPECT_FALSE(result);
    EXPECT_FALSE(parser.isHelpOrVersionRequest());
    EXPECT_TRUE(capturedError.str().find("未知选项") != std::string::npos);
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
    bool result = parser.parse(argc, argv);

    // 恢复标准错误输出
    std::cerr.rdbuf(oldCerr);

    // 验证
    EXPECT_FALSE(result);
    EXPECT_FALSE(parser.isHelpOrVersionRequest());
    EXPECT_TRUE(capturedError.str().find("缺少") != std::string::npos);
}

}  // namespace test
}  // namespace cli
}  // namespace dlogcover

// 添加一个独立的main函数用于覆盖率测试
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
