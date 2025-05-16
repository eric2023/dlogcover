/**
 * @file log_identifier_test.cpp
 * @brief 日志识别器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace core {
namespace log_identifier {
namespace test {

// 创建测试目录和文件的助手函数
class LogIdentifierTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试目录
        testDir_ = std::filesystem::temp_directory_path().string() + "/dlogcover_log_test";
        utils::FileUtils::createDirectory(testDir_);

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
        ASSERT_TRUE(sourceManager_->collectSourceFiles()) << "收集源文件失败";

        // 创建AST分析器
        astAnalyzer_ = std::make_unique<ast_analyzer::ASTAnalyzer>(config_, *sourceManager_);

        // 分析所有文件
        ASSERT_TRUE(astAnalyzer_->analyzeAll()) << "分析所有文件失败";

        // 创建日志识别器
        logIdentifier_ = std::make_unique<LogIdentifier>(config_, *astAnalyzer_);
    }

    void TearDown() override {
        // 清理测试目录和数据
        std::filesystem::remove_all(testDir_);
        logIdentifier_.reset();
        astAnalyzer_.reset();
        sourceManager_.reset();
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
        config.scan.fileTypes = {".cpp", ".h", ".hpp", ".cc", ".c"};

        // 设置Qt日志函数
        config.logFunctions.qt.enabled = true;
        config.logFunctions.qt.functions = {"qDebug", "qInfo", "qWarning", "qCritical", "qFatal"};
        config.logFunctions.qt.categoryFunctions = {"qCDebug", "qCInfo", "qCWarning", "qCCritical"};

        // 设置自定义日志函数
        config.logFunctions.custom.enabled = true;
        config.logFunctions.custom.functions["debug"] = {"debug", "log_debug"};
        config.logFunctions.custom.functions["info"] = {"info", "log_info"};
        config.logFunctions.custom.functions["warning"] = {"warning", "log_warning"};
        config.logFunctions.custom.functions["error"] = {"error", "log_error"};

        return config;
    }

    std::string testDir_;
    config::Config config_;
    std::unique_ptr<source_manager::SourceManager> sourceManager_;
    std::unique_ptr<ast_analyzer::ASTAnalyzer> astAnalyzer_;
    std::unique_ptr<LogIdentifier> logIdentifier_;
};

// 测试初始化和销毁
TEST_F(LogIdentifierTestFixture, InitializeAndDestroy) {
    // 这里主要测试构造和析构是否会导致崩溃
    SUCCEED();
}

// 测试日志函数构建
TEST_F(LogIdentifierTestFixture, LogFunctionNameBuilding) {
    // 验证日志函数名集合
    const auto& logFunctionNames = logIdentifier_->getLogFunctionNames();

    // 检查Qt日志函数
    EXPECT_TRUE(logFunctionNames.count("qDebug") > 0);
    EXPECT_TRUE(logFunctionNames.count("qInfo") > 0);
    EXPECT_TRUE(logFunctionNames.count("qWarning") > 0);
    EXPECT_TRUE(logFunctionNames.count("qCritical") > 0);
    EXPECT_TRUE(logFunctionNames.count("qFatal") > 0);

    // 检查Qt分类日志函数
    EXPECT_TRUE(logFunctionNames.count("qCDebug") > 0);
    EXPECT_TRUE(logFunctionNames.count("qCInfo") > 0);
    EXPECT_TRUE(logFunctionNames.count("qCWarning") > 0);
    EXPECT_TRUE(logFunctionNames.count("qCCritical") > 0);

    // 检查自定义日志函数
    EXPECT_TRUE(logFunctionNames.count("debug") > 0);
    EXPECT_TRUE(logFunctionNames.count("log_debug") > 0);
    EXPECT_TRUE(logFunctionNames.count("info") > 0);
    EXPECT_TRUE(logFunctionNames.count("log_info") > 0);
}

// 测试日志调用识别
TEST_F(LogIdentifierTestFixture, IdentifyLogCalls) {
    // 识别日志调用
    EXPECT_TRUE(logIdentifier_->identifyLogCalls()) << "识别日志调用失败";

    // 获取测试文件的日志调用信息
    std::string testFilePath = testDir_ + "/test.cpp";
    const auto& logCalls = logIdentifier_->getLogCalls(testFilePath);

    // 因为我们使用的是空实现，所以这里实际不会有日志调用被识别出来
    // 在实际实现完成后，可以添加更严格的测试

    // 获取所有日志调用
    const auto& allLogCalls = logIdentifier_->getAllLogCalls();

    // 确保每个文件都被处理
    EXPECT_TRUE(allLogCalls.count(testFilePath) > 0);
}

// 测试Qt日志函数识别
TEST_F(LogIdentifierTestFixture, QtLogFunctionIdentification) {
    // 创建带有各种Qt日志函数的测试文件
    createTestFile(testDir_ + "/qt_log_test.cpp", R"(
#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(TestCategory, "test.category")

void qt_log_test() {
    // 基本日志函数
    qDebug() << "调试信息";
    qInfo() << "普通信息";
    qWarning() << "警告信息";
    qCritical() << "严重错误";

    // 分类日志函数
    qCDebug(TestCategory) << "分类调试信息";
    qCInfo(TestCategory) << "分类普通信息";
    qCWarning(TestCategory) << "分类警告信息";
    qCCritical(TestCategory) << "分类严重错误";

    // 条件日志
    if (true) {
        qDebug() << "条件日志";
    }

    // 循环中的日志
    for (int i = 0; i < 3; ++i) {
        qDebug() << "循环日志" << i;
    }

    // 嵌套日志
    try {
        qDebug() << "外层日志";
        try {
            qWarning() << "内层日志";
            throw std::runtime_error("测试异常");
        } catch (const std::exception& e) {
            qCritical() << "内层异常:" << e.what();
        }
    } catch (...) {
        qFatal() << "外层异常";
    }
}
)");

    // 重新初始化和分析
    sourceManager_->collectSourceFiles();
    astAnalyzer_->analyzeAll();
    EXPECT_TRUE(logIdentifier_->identifyLogCalls()) << "识别日志调用失败";

    // 获取Qt日志测试文件的日志调用信息
    std::string qtTestFilePath = testDir_ + "/qt_log_test.cpp";
    const auto& logCalls = logIdentifier_->getLogCalls(qtTestFilePath);

    // 验证基本日志函数识别
    EXPECT_TRUE(hasLogCall(logCalls, "qDebug", "调试信息"));
    EXPECT_TRUE(hasLogCall(logCalls, "qInfo", "普通信息"));
    EXPECT_TRUE(hasLogCall(logCalls, "qWarning", "警告信息"));
    EXPECT_TRUE(hasLogCall(logCalls, "qCritical", "严重错误"));

    // 验证分类日志函数识别
    EXPECT_TRUE(hasLogCall(logCalls, "qCDebug", "分类调试信息"));
    EXPECT_TRUE(hasLogCall(logCalls, "qCInfo", "分类普通信息"));
    EXPECT_TRUE(hasLogCall(logCalls, "qCWarning", "分类警告信息"));
    EXPECT_TRUE(hasLogCall(logCalls, "qCCritical", "分类严重错误"));
}

// 测试上下文感知的日志识别
TEST_F(LogIdentifierTestFixture, ContextAwareLogIdentification) {
    // 创建带有复杂上下文的测试文件
    createTestFile(testDir_ + "/context_log_test.cpp", R"(
#include <QDebug>

class TestClass {
public:
    void methodWithLogs() {
        qDebug() << "类方法中的日志";

        auto lambda = []() {
            qInfo() << "Lambda中的日志";
        };
        lambda();
    }

    static void staticMethodWithLogs() {
        qWarning() << "静态方法中的日志";
    }
};

namespace test_ns {
void namespaceFunction() {
    qDebug() << "命名空间中的日志";

    class LocalClass {
    public:
        void localMethod() {
            qInfo() << "局部类中的日志";
        }
    };

    LocalClass().localMethod();
}
}

template<typename T>
void templateFunction() {
    qDebug() << "模板函数中的日志";
}

void nestedContextTest() {
    if (true) {
        for (int i = 0; i < 2; ++i) {
            try {
                qDebug() << "嵌套上下文中的日志";
            } catch (...) {
                qCritical() << "异常处理中的日志";
            }
        }
    }
}
)");

    // 重新初始化和分析
    sourceManager_->collectSourceFiles();
    astAnalyzer_->analyzeAll();
    EXPECT_TRUE(logIdentifier_->identifyLogCalls()) << "识别日志调用失败";

    // 获取上下文测试文件的日志调用信息
    std::string contextTestFilePath = testDir_ + "/context_log_test.cpp";
    const auto& logCalls = logIdentifier_->getLogCalls(contextTestFilePath);

    // 验证类方法中的日志
    EXPECT_TRUE(hasLogCallInContext(logCalls, "qDebug", "类方法中的日志", "TestClass::methodWithLogs"));

    // 验证Lambda中的日志
    EXPECT_TRUE(hasLogCallInContext(logCalls, "qInfo", "Lambda中的日志", "TestClass::methodWithLogs"));

    // 验证静态方法中的日志
    EXPECT_TRUE(hasLogCallInContext(logCalls, "qWarning", "静态方法中的日志", "TestClass::staticMethodWithLogs"));

    // 验证命名空间中的日志
    EXPECT_TRUE(hasLogCallInContext(logCalls, "qDebug", "命名空间中的日志", "test_ns::namespaceFunction"));

    // 验证局部类中的日志
    EXPECT_TRUE(hasLogCallInContext(logCalls, "qInfo", "局部类中的日志", "test_ns::namespaceFunction"));

    // 验证模板函数中的日志
    EXPECT_TRUE(hasLogCallInContext(logCalls, "qDebug", "模板函数中的日志", "templateFunction"));

    // 验证嵌套上下文中的日志
    EXPECT_TRUE(hasLogCallInContext(logCalls, "qDebug", "嵌套上下文中的日志", "nestedContextTest"));
    EXPECT_TRUE(hasLogCallInContext(logCalls, "qCritical", "异常处理中的日志", "nestedContextTest"));
}

// 测试日志级别和分类
TEST_F(LogIdentifierTestFixture, LogLevelAndCategoryIdentification) {
    // 创建带有不同日志级别和分类的测试文件
    createTestFile(testDir_ + "/log_level_test.cpp", R"(
#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(NetworkCategory, "network")
Q_LOGGING_CATEGORY(DatabaseCategory, "database")

void testLogLevels() {
    // 基本日志级别
    qDebug() << "调试级别";
    qInfo() << "信息级别";
    qWarning() << "警告级别";
    qCritical() << "严重级别";

    // 带分类的日志级别
    qCDebug(NetworkCategory) << "网络调试";
    qCInfo(NetworkCategory) << "网络信息";
    qCWarning(DatabaseCategory) << "数据库警告";
    qCCritical(DatabaseCategory) << "数据库错误";
}
)");

    // 重新初始化和分析
    sourceManager_->collectSourceFiles();
    astAnalyzer_->analyzeAll();
    EXPECT_TRUE(logIdentifier_->identifyLogCalls()) << "识别日志调用失败";

    // 获取日志级别测试文件的日志调用信息
    std::string levelTestFilePath = testDir_ + "/log_level_test.cpp";
    const auto& logCalls = logIdentifier_->getLogCalls(levelTestFilePath);

    // 验证基本日志级别
    EXPECT_TRUE(hasLogCallWithLevel(logCalls, "qDebug", LogLevel::Debug));
    EXPECT_TRUE(hasLogCallWithLevel(logCalls, "qInfo", LogLevel::Info));
    EXPECT_TRUE(hasLogCallWithLevel(logCalls, "qWarning", LogLevel::Warning));
    EXPECT_TRUE(hasLogCallWithLevel(logCalls, "qCritical", LogLevel::Critical));

    // 验证带分类的日志级别
    EXPECT_TRUE(hasLogCallWithCategory(logCalls, "qCDebug", "network", LogLevel::Debug));
    EXPECT_TRUE(hasLogCallWithCategory(logCalls, "qCInfo", "network", LogLevel::Info));
    EXPECT_TRUE(hasLogCallWithCategory(logCalls, "qCWarning", "database", LogLevel::Warning));
    EXPECT_TRUE(hasLogCallWithCategory(logCalls, "qCCritical", "database", LogLevel::Critical));
}

private:
// 辅助函数：检查是否存在特定的日志调用
bool hasLogCall(const std::vector<LogCall>& calls, const std::string& funcName, const std::string& message) {
    return std::any_of(calls.begin(), calls.end(), [&](const LogCall& call) {
        return call.functionName == funcName && call.message.find(message) != std::string::npos;
    });
}

// 辅助函数：检查是否存在特定上下文中的日志调用
bool hasLogCallInContext(const std::vector<LogCall>& calls, const std::string& funcName, const std::string& message,
                         const std::string& context) {
    return std::any_of(calls.begin(), calls.end(), [&](const LogCall& call) {
        return call.functionName == funcName && call.message.find(message) != std::string::npos &&
               call.context.find(context) != std::string::npos;
    });
}

// 辅助函数：检查是否存在特定级别的日志调用
bool hasLogCallWithLevel(const std::vector<LogCall>& calls, const std::string& funcName, LogLevel level) {
    return std::any_of(calls.begin(), calls.end(),
                       [&](const LogCall& call) { return call.functionName == funcName && call.level == level; });
}

// 辅助函数：检查是否存在特定分类和级别的日志调用
bool hasLogCallWithCategory(const std::vector<LogCall>& calls, const std::string& funcName, const std::string& category,
                            LogLevel level) {
    return std::any_of(calls.begin(), calls.end(), [&](const LogCall& call) {
        return call.functionName == funcName && call.category == category && call.level == level;
    });
}
}  // namespace test
}  // namespace log_identifier
}  // namespace core
}  // namespace dlogcover
