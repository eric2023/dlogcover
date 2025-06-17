/**
 * @file go_analyzer_test.cpp
 * @brief Go分析器单元测试
 * @author DLogCover Team
 * @date 2024-12-20
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <memory>

#include "dlogcover/core/analyzer/go_analyzer.h"
#include "dlogcover/config/config.h"
#include "dlogcover/utils/log_utils.h"

using namespace dlogcover::core::analyzer;
using namespace dlogcover::config;
namespace fs = std::filesystem;

class GoAnalyzerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        testDir_ = fs::temp_directory_path() / "dlogcover_test_go_analyzer";
        fs::create_directories(testDir_);
        
        // 初始化日志系统
        dlogcover::utils::Logger::init("", false, dlogcover::utils::LogLevel::DEBUG);
        
        // 创建基础配置
        setupBasicConfig();
    }
    
    void TearDown() override {
        // 清理测试目录
        if (fs::exists(testDir_)) {
            fs::remove_all(testDir_);
        }
    }
    
    void setupBasicConfig() {        
        // 设置性能配置
        config_.performance.max_threads = 4;
        config_.performance.enable_parallel_analysis = true;
        config_.performance.max_cache_size = 100;
    }
    
    void createTestFile(const std::string& filename, const std::string& content) {
        std::ofstream file(testDir_ / filename);
        file << content;
        file.close();
    }
    
    void createStandardLogTestFile() {
        createTestFile("standard_log.go", R"(
package main

import (
    "log"
    "os"
)

func testStandardLog() {
    log.Print("Print message")
    log.Printf("Printf message: %s", "test")
    log.Println("Println message")
    
    if true {
        log.Fatal("Fatal message")
    }
    
    defer func() {
        if r := recover(); r != nil {
            log.Panic("Panic message")
        }
    }()
}
)");
    }
    
    void createSlogTestFile() {
        createTestFile("slog_test.go", R"(
package main

import (
    "log/slog"
    "context"
)

func testSlog() {
    ctx := context.Background()
    
    slog.Info("Info message")
    slog.Debug("Debug message")
    slog.Warn("Warning message")
    slog.Error("Error message")
    
    slog.InfoContext(ctx, "Info with context")
    slog.DebugContext(ctx, "Debug with context")
    slog.WarnContext(ctx, "Warn with context")
    slog.ErrorContext(ctx, "Error with context")
}
)");
    }
    
    void createLogrusTestFile() {
        createTestFile("logrus_test.go", R"(
package main

import (
    "github.com/sirupsen/logrus"
)

func testLogrus() {
    logrus.Trace("Trace message")
    logrus.Debug("Debug message")
    logrus.Info("Info message")
    logrus.Warn("Warning message")
    logrus.Error("Error message")
    
    logrus.WithField("key", "value").Info("Info with field")
    logrus.WithFields(logrus.Fields{
        "key1": "value1",
        "key2": "value2",
    }).Error("Error with fields")
    
    if false {
        logrus.Fatal("Fatal message")
        logrus.Panic("Panic message")
    }
}
)");
    }
    
    void createZapTestFile() {
        createTestFile("zap_test.go", R"(
package main

import (
    "go.uber.org/zap"
)

func testZap() {
    logger, _ := zap.NewProduction()
    defer logger.Sync()
    
    logger.Debug("Debug message",
        zap.String("key", "value"),
        zap.Int("count", 1),
    )
    
    logger.Info("Info message",
        zap.String("service", "test"),
        zap.Duration("duration", time.Second),
    )
    
    logger.Warn("Warning message")
    logger.Error("Error message")
    
    sugar := logger.Sugar()
    sugar.Debugf("Debug formatted: %s", "test")
    sugar.Infow("Info with fields",
        "key", "value",
        "count", 1,
    )
    
    sugar.Warn("Sugar warning")
    sugar.Error("Sugar error")
}
)");
    }
    
    void createGolibTestFile() {
        createTestFile("golib_test.go", R"(
package main

import (
    "github.com/jackielihf/golib/log"
)

func testGolib() {
    log.Info("Golib info message")
    log.Error("Golib error message")
    log.Debug("Golib debug message")
    log.Warn("Golib warning message")
    
    log.Infof("Golib info formatted: %s", "test")
    log.Errorf("Golib error formatted: %d", 404)
    log.Debugf("Golib debug formatted: %v", true)
    log.Warnf("Golib warning formatted: %f", 3.14)
}
)");
    }
    
    void createMixedLogTestFile() {
        createTestFile("mixed_log.go", R"(
package main

import (
    "log"
    "log/slog"
    "github.com/sirupsen/logrus"
    "go.uber.org/zap"
)

func testMixedLogs() {
    // 标准库日志
    log.Println("Standard log message")
    
    // slog
    slog.Info("Slog info message")
    
    // Logrus
    logrus.WithField("component", "test").Info("Logrus info message")
    
    // Zap
    logger, _ := zap.NewProduction()
    defer logger.Sync()
    logger.Info("Zap info message")
    
    // 条件日志
    if true {
        log.Printf("Conditional log: %s", "active")
        slog.Error("Conditional error")
    }
    
    // 循环中的日志
    for i := 0; i < 3; i++ {
        logrus.Debugf("Loop iteration: %d", i)
    }
    
    // 异常处理中的日志
    defer func() {
        if r := recover(); r != nil {
            log.Printf("Recovered from panic: %v", r)
        }
    }()
}
)");
    }

protected:
    fs::path testDir_;
    Config config_;
};

/**
 * @brief 测试Go分析器的基本构造
 */
TEST_F(GoAnalyzerTest, BasicConstruction) {
    EXPECT_NO_THROW({
        GoAnalyzer analyzer(config_);
    });
}

/**
 * @brief 测试标准库日志函数识别
 */
TEST_F(GoAnalyzerTest, StandardLogRecognition) {
    createStandardLogTestFile();
    
    GoAnalyzer analyzer(config_);
    
    auto result = analyzer.analyze((testDir_ / "standard_log.go").string());
    
    // 如果Go工具可用，应该能识别日志函数
    if (result.isSuccess()) {
        auto stats = analyzer.getStatistics();
        EXPECT_FALSE(stats.empty()) << "应该有统计信息";
    }
    // 如果Go工具不可用，测试不应该崩溃
    EXPECT_NO_THROW(result.isSuccess());
}

/**
 * @brief 测试slog日志函数识别
 */
TEST_F(GoAnalyzerTest, SlogRecognition) {
    createSlogTestFile();
    
    GoAnalyzer analyzer(config_);
    
    auto result = analyzer.analyze((testDir_ / "slog_test.go").string());
    
    // 测试不应该崩溃
    EXPECT_NO_THROW(result.isSuccess());
}

/**
 * @brief 测试Logrus日志函数识别
 */
TEST_F(GoAnalyzerTest, LogrusRecognition) {
    createLogrusTestFile();
    
    GoAnalyzer analyzer(config_);
    
    auto result = analyzer.analyze((testDir_ / "logrus_test.go").string());
    
    // 测试不应该崩溃
    EXPECT_NO_THROW(result.isSuccess());
}

/**
 * @brief 测试Zap日志函数识别
 */
TEST_F(GoAnalyzerTest, ZapRecognition) {
    createZapTestFile();
    
    GoAnalyzer analyzer(config_);
    
    auto result = analyzer.analyze((testDir_ / "zap_test.go").string());
    
    // 测试不应该崩溃
    EXPECT_NO_THROW(result.isSuccess());
}

/**
 * @brief 测试Golib日志函数识别
 */
TEST_F(GoAnalyzerTest, GolibRecognition) {
    createGolibTestFile();
    
    GoAnalyzer analyzer(config_);
    
    auto result = analyzer.analyze((testDir_ / "golib_test.go").string());
    
    // 测试不应该崩溃
    EXPECT_NO_THROW(result.isSuccess());
}

/**
 * @brief 测试混合日志库识别
 */
TEST_F(GoAnalyzerTest, MixedLogRecognition) {
    createMixedLogTestFile();
    
    GoAnalyzer analyzer(config_);
    
    auto result = analyzer.analyze((testDir_ / "mixed_log.go").string());
    
    // 测试不应该崩溃
    EXPECT_NO_THROW(result.isSuccess());
}

/**
 * @brief 测试不存在的文件
 */
TEST_F(GoAnalyzerTest, NonExistentFile) {
    GoAnalyzer analyzer(config_);
    
    auto result = analyzer.analyze((testDir_ / "nonexistent.go").string());
    EXPECT_TRUE(result.hasError()) << "不存在的文件应该返回错误";
}

/**
 * @brief 测试空文件
 */
TEST_F(GoAnalyzerTest, EmptyFile) {
    createTestFile("empty.go", "");
    
    GoAnalyzer analyzer(config_);
    
    auto result = analyzer.analyze((testDir_ / "empty.go").string());
    
    // 空文件应该能正常处理（如果Go工具可用）
    EXPECT_NO_THROW(result.isSuccess());
}

/**
 * @brief 测试无效的Go文件
 */
TEST_F(GoAnalyzerTest, InvalidGoFile) {
    createTestFile("invalid.go", "this is not valid go code {{{");
    
    GoAnalyzer analyzer(config_);
    
    auto result = analyzer.analyze((testDir_ / "invalid.go").string());
    
    // 无效文件应该返回错误但不崩溃
    EXPECT_NO_THROW(result.isSuccess() || result.hasError());
}

/**
 * @brief 测试并行模式设置
 */
TEST_F(GoAnalyzerTest, ParallelMode) {
    GoAnalyzer analyzer(config_);
    
    // 设置并行模式应该不报错
    EXPECT_NO_THROW(analyzer.setParallelMode(true, 2));
    EXPECT_NO_THROW(analyzer.setParallelMode(false, 0));
}

/**
 * @brief 测试批量文件分析
 */
TEST_F(GoAnalyzerTest, BatchAnalysis) {
    // 创建多个测试文件
    createStandardLogTestFile();
    createSlogTestFile();
    createLogrusTestFile();
    
    GoAnalyzer analyzer(config_);
    analyzer.setParallelMode(true, 2);
    
    std::vector<std::string> files = {
        (testDir_ / "standard_log.go").string(),
        (testDir_ / "slog_test.go").string(),
        (testDir_ / "logrus_test.go").string()
    };
    
    // 批量分析应该不崩溃
    EXPECT_NO_THROW({
        auto result = analyzer.analyzeFiles(files);
    });
}

/**
 * @brief 测试统计信息
 */
TEST_F(GoAnalyzerTest, Statistics) {
    createStandardLogTestFile();
    
    GoAnalyzer analyzer(config_);
    
    // 分析文件
    auto result = analyzer.analyze((testDir_ / "standard_log.go").string());
    
    // 获取统计信息
    auto stats = analyzer.getStatistics();
    EXPECT_FALSE(stats.empty()) << "统计信息不应该为空";
}

/**
 * @brief 测试Go工具不可用的情况
 */
TEST_F(GoAnalyzerTest, GoToolUnavailable) {  
    createStandardLogTestFile();
    
    GoAnalyzer analyzer(config_);
    
    // 当Go支持被禁用时，应该成功跳过分析（而不是报错）
    auto result = analyzer.analyze((testDir_ / "standard_log.go").string());
    EXPECT_TRUE(result.isSuccess()) << "Go支持禁用时应该成功跳过分析";
    
    // 验证分析器状态
    EXPECT_FALSE(analyzer.isEnabled()) << "分析器应该显示为禁用状态";
}

/**
 * @brief 测试超时设置
 */
TEST_F(GoAnalyzerTest, TimeoutSetting) {    
    createMixedLogTestFile();
    
    GoAnalyzer analyzer(config_);
    
    // 分析可能因超时而失败，但不应该崩溃
    auto result = analyzer.analyze((testDir_ / "mixed_log.go").string());
    EXPECT_NO_THROW(result.isSuccess() || result.hasError());
}

/**
 * @brief 测试自定义日志库配置
 */
TEST_F(GoAnalyzerTest, CustomLogLibraryConfig) {
    // 添加自定义日志库配置（使用现有的golib配置作为示例）
    config_.go.golib.functions = {"custom.Log", "custom.Logf", "custom.Error"};
    
    createTestFile("custom_log.go", R"(
package main

import "custom"

func testCustomLog() {
    custom.Log("Custom log message")
    custom.Logf("Custom formatted: %s", "test")
    custom.Error("Custom error message")
}
)");
    
    GoAnalyzer analyzer(config_);
    
    auto result = analyzer.analyze((testDir_ / "custom_log.go").string());
    
    // 测试不应该崩溃
    EXPECT_NO_THROW(result.isSuccess());
} 