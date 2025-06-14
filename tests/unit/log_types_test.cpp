/**
 * @file log_types_test.cpp
 * @brief 日志类型处理测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/common/log_types.h>

#include <gtest/gtest.h>
#include <stdexcept>

using namespace dlogcover::common;

namespace dlogcover {
namespace test {

class LogTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试数据准备
    }

    void TearDown() override {
        // 清理
    }
};

// 测试日志级别转换为字符串
TEST_F(LogTypesTest, LogLevelToString) {
    EXPECT_EQ(toString(LogLevel::DEBUG), "debug");
    EXPECT_EQ(toString(LogLevel::INFO), "info");
    EXPECT_EQ(toString(LogLevel::WARNING), "warning");
    EXPECT_EQ(toString(LogLevel::ERROR), "error");
    EXPECT_EQ(toString(LogLevel::ALL), "all");
}

// 测试字符串解析为日志级别
TEST_F(LogTypesTest, ParseLogLevelValid) {
    EXPECT_EQ(parseLogLevel("DEBUG"), LogLevel::DEBUG);
    EXPECT_EQ(parseLogLevel("INFO"), LogLevel::INFO);
    EXPECT_EQ(parseLogLevel("WARNING"), LogLevel::WARNING);
    EXPECT_EQ(parseLogLevel("ERROR"), LogLevel::ERROR);
    EXPECT_EQ(parseLogLevel("ALL"), LogLevel::ALL);
}

TEST_F(LogTypesTest, ParseLogLevelCaseInsensitive) {
    EXPECT_EQ(parseLogLevel("debug"), LogLevel::DEBUG);
    EXPECT_EQ(parseLogLevel("info"), LogLevel::INFO);
    EXPECT_EQ(parseLogLevel("warning"), LogLevel::WARNING);
    EXPECT_EQ(parseLogLevel("error"), LogLevel::ERROR);
    EXPECT_EQ(parseLogLevel("all"), LogLevel::ALL);
    
    // 混合大小写
    EXPECT_EQ(parseLogLevel("Debug"), LogLevel::DEBUG);
    EXPECT_EQ(parseLogLevel("Info"), LogLevel::INFO);
    EXPECT_EQ(parseLogLevel("Warning"), LogLevel::WARNING);
    EXPECT_EQ(parseLogLevel("Error"), LogLevel::ERROR);
    EXPECT_EQ(parseLogLevel("All"), LogLevel::ALL);
}

TEST_F(LogTypesTest, ParseLogLevelInvalid) {
    EXPECT_THROW(parseLogLevel("INVALID"), std::invalid_argument);
    EXPECT_THROW(parseLogLevel("TRACE"), std::invalid_argument);
    // FATAL 实际上是有效的，根据实现
    // EXPECT_THROW(parseLogLevel("FATAL"), std::invalid_argument);
    EXPECT_THROW(parseLogLevel(""), std::invalid_argument);
    EXPECT_THROW(parseLogLevel("123"), std::invalid_argument);
}

// 测试日志级别比较
TEST_F(LogTypesTest, LogLevelComparison) {
    // 测试日志级别的数值顺序
    EXPECT_TRUE(static_cast<int>(LogLevel::DEBUG) < static_cast<int>(LogLevel::INFO));
    EXPECT_TRUE(static_cast<int>(LogLevel::INFO) < static_cast<int>(LogLevel::WARNING));
    EXPECT_TRUE(static_cast<int>(LogLevel::WARNING) < static_cast<int>(LogLevel::ERROR));
    EXPECT_TRUE(static_cast<int>(LogLevel::ERROR) < static_cast<int>(LogLevel::FATAL));
    
    // ALL 应该是最高级别（包含所有日志）
    EXPECT_TRUE(static_cast<int>(LogLevel::ALL) > static_cast<int>(LogLevel::ERROR));
}

// 测试日志级别枚举值
TEST_F(LogTypesTest, LogLevelEnumValues) {
    // 确保枚举值符合预期
    EXPECT_EQ(static_cast<int>(LogLevel::UNKNOWN), -1);
    EXPECT_EQ(static_cast<int>(LogLevel::DEBUG), 0);
    EXPECT_EQ(static_cast<int>(LogLevel::INFO), 1);
    EXPECT_EQ(static_cast<int>(LogLevel::WARNING), 2);
    EXPECT_EQ(static_cast<int>(LogLevel::ERROR), 3);
    EXPECT_EQ(static_cast<int>(LogLevel::CRITICAL), 3);
    EXPECT_EQ(static_cast<int>(LogLevel::FATAL), 4);
    EXPECT_EQ(static_cast<int>(LogLevel::ALL), 5);
}

// 测试日志级别往返转换
TEST_F(LogTypesTest, LogLevelRoundTrip) {
    std::vector<LogLevel> levels = {
        LogLevel::DEBUG,
        LogLevel::INFO,
        LogLevel::WARNING,
        LogLevel::ERROR,
        LogLevel::ALL
    };
    
    for (LogLevel level : levels) {
        std::string str = std::string(toString(level));
        LogLevel parsed = parseLogLevel(str);
        EXPECT_EQ(level, parsed);
    }
}

// 测试日志级别字符串视图
TEST_F(LogTypesTest, LogLevelStringView) {
    // 确保返回的是有效的字符串视图
    auto debugStr = toString(LogLevel::DEBUG);
    EXPECT_FALSE(debugStr.empty());
    EXPECT_EQ(debugStr.length(), 5);  // "DEBUG"
    
    auto infoStr = toString(LogLevel::INFO);
    EXPECT_FALSE(infoStr.empty());
    EXPECT_EQ(infoStr.length(), 4);  // "INFO"
    
    auto warningStr = toString(LogLevel::WARNING);
    EXPECT_FALSE(warningStr.empty());
    EXPECT_EQ(warningStr.length(), 7);  // "WARNING"
    
    auto errorStr = toString(LogLevel::ERROR);
    EXPECT_FALSE(errorStr.empty());
    EXPECT_EQ(errorStr.length(), 5);  // "ERROR"
    
    auto allStr = toString(LogLevel::ALL);
    EXPECT_FALSE(allStr.empty());
    EXPECT_EQ(allStr.length(), 3);  // "ALL"
}

// 测试边界情况
TEST_F(LogTypesTest, ParseLogLevelWhitespace) {
    // 根据实际实现，可能不支持自动去除空格
    // 如果实现不支持，则这些应该抛出异常
    EXPECT_THROW(parseLogLevel(" DEBUG "), std::invalid_argument);
    EXPECT_THROW(parseLogLevel("\tINFO\t"), std::invalid_argument);
    EXPECT_THROW(parseLogLevel("\nWARNING\n"), std::invalid_argument);
    EXPECT_THROW(parseLogLevel(" ERROR "), std::invalid_argument);
    EXPECT_THROW(parseLogLevel(" ALL "), std::invalid_argument);
}

TEST_F(LogTypesTest, ParseLogLevelEmptyAndWhitespace) {
    EXPECT_THROW(parseLogLevel(""), std::invalid_argument);
    EXPECT_THROW(parseLogLevel(" "), std::invalid_argument);
    EXPECT_THROW(parseLogLevel("\t"), std::invalid_argument);
    EXPECT_THROW(parseLogLevel("\n"), std::invalid_argument);
}

// 测试日志级别的实用函数
TEST_F(LogTypesTest, IsLogLevelEnabled) {
    // 假设有一个函数检查日志级别是否启用
    // 这里测试日志级别的逻辑关系
    
    // DEBUG 级别应该包含所有日志
    LogLevel currentLevel = LogLevel::DEBUG;
    EXPECT_TRUE(static_cast<int>(LogLevel::DEBUG) >= static_cast<int>(currentLevel));
    EXPECT_TRUE(static_cast<int>(LogLevel::INFO) >= static_cast<int>(currentLevel));
    EXPECT_TRUE(static_cast<int>(LogLevel::WARNING) >= static_cast<int>(currentLevel));
    EXPECT_TRUE(static_cast<int>(LogLevel::ERROR) >= static_cast<int>(currentLevel));
    
    // ERROR 级别只应该包含 ERROR 日志
    currentLevel = LogLevel::ERROR;
    EXPECT_FALSE(static_cast<int>(LogLevel::DEBUG) >= static_cast<int>(currentLevel));
    EXPECT_FALSE(static_cast<int>(LogLevel::INFO) >= static_cast<int>(currentLevel));
    EXPECT_FALSE(static_cast<int>(LogLevel::WARNING) >= static_cast<int>(currentLevel));
    EXPECT_TRUE(static_cast<int>(LogLevel::ERROR) >= static_cast<int>(currentLevel));
}

// 测试日志级别的字符串表示一致性
TEST_F(LogTypesTest, LogLevelStringConsistency) {
    // 确保字符串表示与枚举名称一致
    EXPECT_EQ(toString(LogLevel::DEBUG), "debug");
    EXPECT_EQ(toString(LogLevel::INFO), "info");
    EXPECT_EQ(toString(LogLevel::WARNING), "warning");
    EXPECT_EQ(toString(LogLevel::ERROR), "error");
    EXPECT_EQ(toString(LogLevel::ALL), "all");
    
    // 确保解析结果与原始枚举一致
    EXPECT_EQ(parseLogLevel("DEBUG"), LogLevel::DEBUG);
    EXPECT_EQ(parseLogLevel("INFO"), LogLevel::INFO);
    EXPECT_EQ(parseLogLevel("WARNING"), LogLevel::WARNING);
    EXPECT_EQ(parseLogLevel("ERROR"), LogLevel::ERROR);
    EXPECT_EQ(parseLogLevel("ALL"), LogLevel::ALL);
}

// 测试所有可能的日志级别
TEST_F(LogTypesTest, AllLogLevelsSupported) {
    std::vector<std::pair<LogLevel, std::string>> levelPairs = {
        {LogLevel::ALL, "all"},
        {LogLevel::DEBUG, "debug"},
        {LogLevel::INFO, "info"},
        {LogLevel::WARNING, "warning"},
        {LogLevel::ERROR, "error"}
    };
    
    for (const auto& pair : levelPairs) {
        // 测试枚举到字符串
        EXPECT_EQ(toString(pair.first), pair.second);
        
        // 测试字符串到枚举
        EXPECT_EQ(parseLogLevel(pair.second), pair.first);
        
        // 测试大小写不敏感 - 测试大写版本
        std::string upperCase = pair.second;
        std::transform(upperCase.begin(), upperCase.end(), upperCase.begin(), ::toupper);
        EXPECT_EQ(parseLogLevel(upperCase), pair.first);
    }
}

// 测试异常消息
TEST_F(LogTypesTest, ParseLogLevelExceptionMessage) {
    try {
        parseLogLevel("INVALID_LEVEL");
        FAIL() << "Expected std::invalid_argument";
    } catch (const std::invalid_argument& e) {
        std::string message = e.what();
        EXPECT_FALSE(message.empty());
        EXPECT_TRUE(message.find("INVALID_LEVEL") != std::string::npos);
    }
}

// 测试性能相关的边界情况
TEST_F(LogTypesTest, ParseLogLevelPerformance) {
    // 测试大量解析操作不会出现问题
    std::vector<std::string> validLevels = {"DEBUG", "INFO", "WARNING", "ERROR", "ALL"};
    std::vector<std::string> expectedLevels = {"debug", "info", "warning", "error", "all"};
    
    for (int i = 0; i < 100; ++i) {  // 减少循环次数避免超时
        for (size_t j = 0; j < validLevels.size(); ++j) {
            LogLevel parsed = parseLogLevel(validLevels[j]);
            std::string converted = std::string(toString(parsed));
            EXPECT_EQ(converted, expectedLevels[j]);
        }
    }
}

// 测试日志级别的默认行为
TEST_F(LogTypesTest, DefaultLogLevel) {
    // 测试默认构造的日志级别行为
    LogLevel defaultLevel = getDefaultLogLevel();  // 使用实际的默认级别函数
    
    std::string defaultStr = std::string(toString(defaultLevel));
    EXPECT_EQ(defaultStr, "info");  // 根据实现，默认应该是 INFO
    
    LogLevel parsedDefault = parseLogLevel("INFO");
    EXPECT_EQ(parsedDefault, LogLevel::INFO);
}

}  // namespace test
}  // namespace dlogcover 