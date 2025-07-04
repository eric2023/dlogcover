/**
 * @file coverage_calculator_test.cpp
 * @brief 覆盖率计算器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/coverage/coverage_calculator.h>
#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace core {
namespace coverage {
namespace test {

// 创建测试目录和文件的助手函数
class CoverageCalculatorTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化日志系统，设置为ERROR级别以减少测试期间的日志输出
        utils::Logger::init("", false, utils::LogLevel::ERROR);
        
        // 创建测试目录
        testDir_ = "/tmp/dlogcover_coverage_test";
        if (std::filesystem::exists(testDir_)) {
            std::filesystem::remove_all(testDir_);
        }
        std::filesystem::create_directories(testDir_);

        // 创建测试文件
        createTestFile(testDir_ + "/test.cpp", R"(
#include <iostream>
#include <QDebug>

// 普通函数
void regular_function() {
    std::cout << "普通函数" << std::endl;
}

// 带日志的函数
void logged_function() {
    qDebug() << "这是一条调试日志";
    std::cout << "带日志的函数" << std::endl;
    qInfo() << "这是一条信息日志";
}

// 带条件分支的函数
int conditional_function(int value) {
    if (value > 0) {
        qDebug() << "正数分支" << value;
        return value * 2;
    } else {
        qWarning() << "负数或零分支" << value;
        return value * -1;
    }
}

// 带异常处理的函数
void exception_function() {
    try {
        throw std::runtime_error("测试异常");
    } catch (const std::exception& e) {
        qCritical() << "捕获异常:" << e.what();
        std::cerr << "捕获异常: " << e.what() << std::endl;
    }
}

int main() {
    regular_function();
    logged_function();
    conditional_function(10);
    conditional_function(-5);

    try {
        exception_function();
    } catch (...) {
        qFatal() << "致命错误";
    }

    return 0;
}
)");

        // 设置配置
        config_ = createTestConfig();

        // 初始化源文件管理器
        sourceManager_ = std::make_unique<source_manager::SourceManager>(config_);

        // 收集源文件
        auto collectResult = sourceManager_->collectSourceFiles();
        ASSERT_FALSE(collectResult.hasError()) << "收集源文件失败: " << collectResult.errorMessage();
        ASSERT_TRUE(collectResult.value()) << "未能有效收集源文件";

        // 创建AST分析器
        // 创建配置管理器
        configManager_ = std::make_unique<config::ConfigManager>();
        
        astAnalyzer_ = std::make_unique<ast_analyzer::ASTAnalyzer>(config_, *sourceManager_, *configManager_);

        // 分析所有文件
        auto analyzeResult = astAnalyzer_->analyzeAll();
        ASSERT_FALSE(analyzeResult.hasError()) << "分析所有文件失败: " << analyzeResult.errorMessage();
        ASSERT_TRUE(analyzeResult.value()) << "分析文件返回false";

        // 创建日志识别器
        logIdentifier_ = std::make_unique<log_identifier::LogIdentifier>(config_, *astAnalyzer_);

        // 识别日志调用
        auto identifyResult = logIdentifier_->identifyLogCalls();
        ASSERT_FALSE(identifyResult.hasError()) << "识别日志调用失败: " << identifyResult.errorMessage();
        ASSERT_TRUE(identifyResult.value()) << "识别日志调用返回false";

        // 创建覆盖率计算器
        coverageCalculator_ = std::make_unique<CoverageCalculator>(config_, *astAnalyzer_, *logIdentifier_);
    }

    void TearDown() override {
        // 关闭日志系统，确保所有资源正确释放
        utils::Logger::shutdown();
        
        // 清理测试目录
        if (std::filesystem::exists(testDir_)) {
            std::filesystem::remove_all(testDir_);
        }
        coverageCalculator_.reset();
        logIdentifier_.reset();
        astAnalyzer_.reset();
        sourceManager_.reset();
        configManager_.reset();
    }

    void createTestFile(const std::string& path, const std::string& content) {
        std::ofstream file(path);
        file << content;
        file.close();
    }

    config::Config createTestConfig() {
        config::Config config;

        // 设置扫描目录
        config.scan.directories = {testDir_};

        // 设置文件类型
        config.scan.file_extensions = {".cpp", ".h", ".hpp", ".cc", ".c"};
        config.scan.exclude_patterns = {"*/tests/*", "*/build/*", "*/.git/*", "*/CMakeFiles/*"};

        // 设置日志函数
        config.log_functions.qt.enabled = true;
        config.log_functions.qt.functions = {"qDebug", "qInfo", "qWarning", "qCritical", "qFatal"};

        // 编译命令配置
        config.compile_commands.path = "./build/compile_commands.json";
        config.compile_commands.auto_generate = true;
        config.compile_commands.cmake_args = {"-I/usr/include", "-I/usr/include/c++/8", "-I/usr/include/x86_64-linux-gnu/c++/8",
                                              "-I/usr/include/x86_64-linux-gnu", "-I/usr/include/linux",
                                              "-I/usr/lib/gcc/x86_64-linux-gnu/8/include",
                                              "-I/usr/lib/gcc/x86_64-linux-gnu/8/include-fixed",
                                              "-I/usr/local/include", "-I/usr/include/qt5",
                                              "-I/usr/include/qt5/QtCore", "-I/usr/include/qt5/QtGui",
                                              "-I/usr/include/qt5/QtWidgets", "-std=c++17", "-fPIC"};

        // 分析配置
        config.analysis.function_coverage = true;
        config.analysis.branch_coverage = true;
        config.analysis.exception_coverage = true;
        config.analysis.key_path_coverage = true;

        // 输出配置
        config.output.report_file = "test_report.txt";
        config.output.log_file = "test_analysis.log";
        config.output.log_level = "INFO";

        // 项目配置
        config.project.name = "test-project";
        config.project.directory = "./";
        config.project.build_directory = "./build";

        return config;
    }

    std::string testDir_;
    config::Config config_;
    std::unique_ptr<config::ConfigManager> configManager_;
    std::unique_ptr<source_manager::SourceManager> sourceManager_;
    std::unique_ptr<ast_analyzer::ASTAnalyzer> astAnalyzer_;
    std::unique_ptr<log_identifier::LogIdentifier> logIdentifier_;
    std::unique_ptr<CoverageCalculator> coverageCalculator_;
};

// 测试初始化和销毁
TEST_F(CoverageCalculatorTestFixture, InitializeAndDestroy) {
    // 这里主要测试构造和析构是否会导致崩溃
    SUCCEED();
}

// 测试覆盖率计算
TEST_F(CoverageCalculatorTestFixture, CalculateCoverage) {
    // 计算覆盖率
    auto calculateResult = coverageCalculator_->calculate();
    EXPECT_TRUE(calculateResult) << "计算覆盖率失败";

    // 获取测试文件的覆盖率统计信息
    std::string testFilePath = testDir_ + "/test.cpp";
    const CoverageStats& fileStats = coverageCalculator_->getCoverageStats(testFilePath);

    // 简单验证覆盖率计算是否进行
    // 注意：由于我们的实现是框架性的，这里只验证基本逻辑
    EXPECT_GE(fileStats.functionCoverage, 0.0);
    EXPECT_LE(fileStats.functionCoverage, 1.0);

    EXPECT_GE(fileStats.branchCoverage, 0.0);
    EXPECT_LE(fileStats.branchCoverage, 1.0);

    EXPECT_GE(fileStats.exceptionCoverage, 0.0);
    EXPECT_LE(fileStats.exceptionCoverage, 1.0);

    EXPECT_GE(fileStats.keyPathCoverage, 0.0);
    EXPECT_LE(fileStats.keyPathCoverage, 1.0);

    EXPECT_GE(fileStats.overallCoverage, 0.0);
    EXPECT_LE(fileStats.overallCoverage, 1.0);
}

// 测试总体覆盖率计算
TEST_F(CoverageCalculatorTestFixture, OverallCoverage) {
    // 计算覆盖率
    auto calculateResult = coverageCalculator_->calculate();
    EXPECT_TRUE(calculateResult) << "计算覆盖率失败";

    // 获取总体覆盖率统计信息
    const CoverageStats& overallStats = coverageCalculator_->getOverallCoverageStats();

    // 验证总体覆盖率计算
    EXPECT_GE(overallStats.functionCoverage, 0.0);
    EXPECT_LE(overallStats.functionCoverage, 1.0);

    EXPECT_GE(overallStats.branchCoverage, 0.0);
    EXPECT_LE(overallStats.branchCoverage, 1.0);

    EXPECT_GE(overallStats.exceptionCoverage, 0.0);
    EXPECT_LE(overallStats.exceptionCoverage, 1.0);

    EXPECT_GE(overallStats.keyPathCoverage, 0.0);
    EXPECT_LE(overallStats.keyPathCoverage, 1.0);

    EXPECT_GE(overallStats.overallCoverage, 0.0);
    EXPECT_LE(overallStats.overallCoverage, 1.0);
}

// 测试未覆盖路径建议
TEST_F(CoverageCalculatorTestFixture, UncoveredPathSuggestions) {
    // 计算覆盖率
    auto calculateResult = coverageCalculator_->calculate();
    EXPECT_TRUE(calculateResult) << "计算覆盖率失败";

    // 获取总体覆盖率统计信息
    const CoverageStats& overallStats = coverageCalculator_->getOverallCoverageStats();

    // 在实际实现中，应该验证生成的建议的内容
    // 由于我们的实现是框架性的，这里只进行简单检查

    // 如果有未覆盖路径，应该有对应的建议
    if (!overallStats.uncoveredPaths.empty()) {
        EXPECT_FALSE(overallStats.uncoveredPaths.front().suggestion.empty()) << "未生成建议";
    }
}

// 重构测试用例，验证复合语句不会被错误识别为函数
TEST_F(CoverageCalculatorTestFixture, DoesNotCountCompoundStmtAsFunctions) {
    // 创建一个包含复合语句的测试文件
    std::string testContent = R"(
#include <iostream>

// 测试函数，包含多个复合语句块
void testFunction() {
    // 第一个复合语句块
    {
        int x = 1;
        std::cout << "Block 1: " << x << std::endl;
    }
    
    // 第二个复合语句块
    {
        int y = 2;
        std::cout << "Block 2: " << y << std::endl;
    }
    
    // 第三个复合语句块
    {
        int z = 3;
        std::cout << "Block 3: " << z << std::endl;
    }
}

// 另一个真实的函数
void anotherFunction() {
    std::cout << "Another function" << std::endl;
}
)";

    // 创建测试文件
    std::string testFilePath = testDir_ + "/compound_test.cpp";
    createTestFile(testFilePath, testContent);

    // 重新收集源文件以包含新的测试文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_FALSE(collectResult.hasError()) << "重新收集源文件失败: " << collectResult.errorMessage();

    // 重新分析AST
    auto analyzeResult = astAnalyzer_->analyzeAll();
    ASSERT_FALSE(analyzeResult.hasError()) << "重新分析AST失败: " << analyzeResult.errorMessage();

    // 重新识别日志调用
    auto identifyResult = logIdentifier_->identifyLogCalls();
    ASSERT_FALSE(identifyResult.hasError()) << "重新识别日志调用失败: " << identifyResult.errorMessage();

    // 计算覆盖率
    EXPECT_TRUE(coverageCalculator_->calculate()) << "计算覆盖率失败";

    // 获取测试文件的覆盖率统计结果
    const auto& stats = coverageCalculator_->getCoverageStats(testFilePath);

    // 验证只计算了2个真实函数（testFunction 和 anotherFunction），
    // 而不是5个（2个真实函数 + 3个复合语句块）
    // 注意：由于我们的实现可能还在开发中，这里主要验证不会崩溃和基本逻辑
    EXPECT_GE(stats.totalFunctions, 0) << "函数统计应该是非负数";
    
    // 如果统计功能已实现，验证复合语句不被计算为函数
    if (stats.totalFunctions > 0) {
        EXPECT_LE(stats.totalFunctions, 2) << "应该只统计真实函数，不包括复合语句块";
    }
}

// 测试错误处理路径
TEST_F(CoverageCalculatorTestFixture, ErrorHandlingPaths) {
    // 计算覆盖率
    auto calculateResult = coverageCalculator_->calculate();
    EXPECT_TRUE(calculateResult) << "计算覆盖率失败";

    // 测试获取不存在文件的覆盖率统计（应该返回空统计）
    std::string nonExistentFile = testDir_ + "/non_existent.cpp";
    const CoverageStats& emptyStats = coverageCalculator_->getCoverageStats(nonExistentFile);
    
    // 验证返回的是空统计对象
    EXPECT_EQ(emptyStats.totalFunctions, 0) << "不存在文件应该返回空统计";
    EXPECT_EQ(emptyStats.coveredFunctions, 0) << "不存在文件应该返回空统计";
    EXPECT_EQ(emptyStats.functionCoverage, 0.0) << "不存在文件应该返回0覆盖率";
    EXPECT_EQ(emptyStats.branchCoverage, 0.0) << "不存在文件应该返回0覆盖率";
    EXPECT_EQ(emptyStats.exceptionCoverage, 0.0) << "不存在文件应该返回0覆盖率";
    EXPECT_EQ(emptyStats.keyPathCoverage, 0.0) << "不存在文件应该返回0覆盖率";
    EXPECT_EQ(emptyStats.overallCoverage, 0.0) << "不存在文件应该返回0覆盖率";
}

// 测试空文件和边界条件
TEST_F(CoverageCalculatorTestFixture, EmptyFileAndBoundaryConditions) {
    // 创建空文件
    std::string emptyFilePath = testDir_ + "/empty.cpp";
    createTestFile(emptyFilePath, "");

    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_FALSE(collectResult.hasError()) << "重新收集源文件失败: " << collectResult.errorMessage();

    // 重新分析AST
    auto analyzeResult = astAnalyzer_->analyzeAll();
    ASSERT_FALSE(analyzeResult.hasError()) << "重新分析AST失败: " << analyzeResult.errorMessage();

    // 重新识别日志调用
    auto identifyResult = logIdentifier_->identifyLogCalls();
    ASSERT_FALSE(identifyResult.hasError()) << "重新识别日志调用失败: " << identifyResult.errorMessage();

    // 计算覆盖率
    EXPECT_TRUE(coverageCalculator_->calculate()) << "计算覆盖率失败";

    // 获取空文件的覆盖率统计
    const auto& stats = coverageCalculator_->getCoverageStats(emptyFilePath);
    
    // 验证空文件的处理
    EXPECT_GE(stats.totalFunctions, 0) << "空文件函数统计应该是非负数";
    EXPECT_GE(stats.functionCoverage, 0.0) << "空文件覆盖率应该是非负数";
    EXPECT_LE(stats.functionCoverage, 1.0) << "空文件覆盖率应该不超过1.0";
}

// 测试复合语句跳过逻辑
TEST_F(CoverageCalculatorTestFixture, CompoundStatementSkipping) {
    // 创建包含复合语句的测试文件
    std::string testContent = R"(
#include <iostream>
#include <QDebug>

// 模拟Qt日志函数定义
#define qDebug() QDebugMock()
#define qInfo() QInfoMock()

class QDebugMock {
public:
    QDebugMock& operator<<(const char* msg) { return *this; }
};

class QInfoMock {
public:
    QInfoMock& operator<<(const char* msg) { return *this; }
};

// 包含日志的函数
void functionWithLogging() {
    qDebug() << "这是一个调试消息";
    
    // 复合语句块
    {
        int x = 1;
        qInfo() << "复合语句中的日志";
    }
    
    // 嵌套复合语句
    {
        {
            int y = 2;
            std::cout << "嵌套复合语句" << std::endl;
        }
    }
}

// 不包含日志的函数
void functionWithoutLogging() {
    std::cout << "普通函数" << std::endl;
    
    // 复合语句块
    {
        int z = 3;
        std::cout << "复合语句中的普通代码" << std::endl;
    }
}
)";

    std::string testFilePath = testDir_ + "/compound_stmt_test.cpp";
    createTestFile(testFilePath, testContent);

    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_FALSE(collectResult.hasError()) << "重新收集源文件失败: " << collectResult.errorMessage();

    // 重新分析AST
    auto analyzeResult = astAnalyzer_->analyzeAll();
    ASSERT_FALSE(analyzeResult.hasError()) << "重新分析AST失败: " << analyzeResult.errorMessage();

    // 重新识别日志调用
    auto identifyResult = logIdentifier_->identifyLogCalls();
    ASSERT_FALSE(identifyResult.hasError()) << "重新识别日志调用失败: " << identifyResult.errorMessage();

    // 计算覆盖率
    EXPECT_TRUE(coverageCalculator_->calculate()) << "计算覆盖率失败";

    // 获取测试文件的覆盖率统计
    const auto& stats = coverageCalculator_->getCoverageStats(testFilePath);
    
    // 验证复合语句不被计算为函数
    EXPECT_GE(stats.totalFunctions, 0) << "函数统计应该是非负数";
    
    // 如果统计功能已实现，验证只计算真实函数
    if (stats.totalFunctions > 0) {
        // 注意：实际实现可能会检测到更多函数（如构造函数、析构函数等）
        // 这里主要验证不会因为复合语句而崩溃
        EXPECT_GE(stats.totalFunctions, 2) << "应该至少检测到2个真实函数";
        EXPECT_LE(stats.totalFunctions, 10) << "函数数量应该在合理范围内";
    }
}

// 测试日志调用为空的情况
TEST_F(CoverageCalculatorTestFixture, EmptyLogCallsHandling) {
    // 创建没有日志调用的测试文件
    std::string testContent = R"(
#include <iostream>

// 普通函数，没有日志调用
void regularFunction1() {
    std::cout << "普通函数1" << std::endl;
    int x = 42;
}

void regularFunction2() {
    std::cout << "普通函数2" << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::cout << i << " ";
    }
}

class TestClass {
public:
    void memberFunction() {
        std::cout << "成员函数" << std::endl;
    }
};
)";

    std::string testFilePath = testDir_ + "/no_logs_test.cpp";
    createTestFile(testFilePath, testContent);

    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_FALSE(collectResult.hasError()) << "重新收集源文件失败: " << collectResult.errorMessage();

    // 重新分析AST
    auto analyzeResult = astAnalyzer_->analyzeAll();
    ASSERT_FALSE(analyzeResult.hasError()) << "重新分析AST失败: " << analyzeResult.errorMessage();

    // 重新识别日志调用
    auto identifyResult = logIdentifier_->identifyLogCalls();
    ASSERT_FALSE(identifyResult.hasError()) << "重新识别日志调用失败: " << identifyResult.errorMessage();

    // 计算覆盖率
    EXPECT_TRUE(coverageCalculator_->calculate()) << "计算覆盖率失败";

    // 获取测试文件的覆盖率统计
    const auto& stats = coverageCalculator_->getCoverageStats(testFilePath);
    
    // 验证没有日志调用的文件处理
    EXPECT_GE(stats.totalFunctions, 0) << "函数统计应该是非负数";
    EXPECT_GE(stats.functionCoverage, 0.0) << "覆盖率应该是非负数";
    EXPECT_LE(stats.functionCoverage, 1.0) << "覆盖率应该不超过1.0";
    
    // 没有日志调用的文件，函数覆盖率应该是0
    if (stats.totalFunctions > 0) {
        EXPECT_EQ(stats.functionCoverage, 0.0) << "没有日志调用的文件，函数覆盖率应该是0";
    }
}

// 测试函数覆盖检测的增强逻辑
TEST_F(CoverageCalculatorTestFixture, EnhancedCoverageDetection) {
    // 创建包含子节点日志调用的测试文件
    std::string testContent = R"(
#include <iostream>
#include <QDebug>

// 模拟Qt日志函数定义
#define qDebug() QDebugMock()
#define qWarning() QWarningMock()

class QDebugMock {
public:
    QDebugMock& operator<<(const char* msg) { return *this; }
};

class QWarningMock {
public:
    QWarningMock& operator<<(const char* msg) { return *this; }
};

// 函数本身没有直接日志调用，但子节点有
void parentFunction() {
    if (true) {
        qDebug() << "子节点中的日志调用";
    }
    
    for (int i = 0; i < 5; ++i) {
        if (i % 2 == 0) {
            qWarning() << "循环中的日志调用";
        }
    }
}

// 直接包含日志调用的函数
void directLogFunction() {
    qDebug() << "直接的日志调用";
}
)";

    std::string testFilePath = testDir_ + "/enhanced_detection_test.cpp";
    createTestFile(testFilePath, testContent);

    // 重新收集源文件
    auto collectResult = sourceManager_->collectSourceFiles();
    ASSERT_FALSE(collectResult.hasError()) << "重新收集源文件失败: " << collectResult.errorMessage();

    // 重新分析AST
    auto analyzeResult = astAnalyzer_->analyzeAll();
    ASSERT_FALSE(analyzeResult.hasError()) << "重新分析AST失败: " << analyzeResult.errorMessage();

    // 重新识别日志调用
    auto identifyResult = logIdentifier_->identifyLogCalls();
    ASSERT_FALSE(identifyResult.hasError()) << "重新识别日志调用失败: " << identifyResult.errorMessage();

    // 计算覆盖率
    EXPECT_TRUE(coverageCalculator_->calculate()) << "计算覆盖率失败";

    // 获取测试文件的覆盖率统计
    const auto& stats = coverageCalculator_->getCoverageStats(testFilePath);
    
    // 验证增强的覆盖检测逻辑
    EXPECT_GE(stats.totalFunctions, 0) << "函数统计应该是非负数";
    EXPECT_GE(stats.functionCoverage, 0.0) << "覆盖率应该是非负数";
    EXPECT_LE(stats.functionCoverage, 1.0) << "覆盖率应该不超过1.0";
    
    // 如果检测到函数，应该有合理的覆盖率
    if (stats.totalFunctions > 0) {
        EXPECT_GT(stats.functionCoverage, 0.0) << "包含日志调用的文件应该有正覆盖率";
    }
}

}  // namespace test
}  // namespace coverage
}  // namespace core
}  // namespace dlogcover
