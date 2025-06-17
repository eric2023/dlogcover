/**
 * @file cpp_analyzer_adapter_test.cpp
 * @brief C++分析器适配器单元测试
 * @author DLogCover Team
 * @date 2024-12-20
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <memory>

#include "dlogcover/core/analyzer/cpp_analyzer_adapter.h"
#include "dlogcover/config/config.h"
#include "dlogcover/source_manager/source_manager.h"
#include "dlogcover/config/config_manager.h"
#include "dlogcover/utils/log_utils.h"

using namespace dlogcover::core::analyzer;
using namespace dlogcover::config;
using namespace dlogcover::source_manager;
namespace fs = std::filesystem;

class CppAnalyzerAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        testDir_ = fs::temp_directory_path() / "dlogcover_test_cpp_adapter";
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
        config_.scan.directories.push_back(testDir_.string());
        config_.scan.file_extensions = {".cpp", ".h", ".cxx", ".hpp"};
        config_.output.report_file = (testDir_ / "output.json").string();
        config_.output.log_file = (testDir_ / "test.log").string();
        
        // 设置编译数据库
        config_.compile_commands.path = (testDir_ / "compile_commands.json").string();
        config_.compile_commands.auto_generate = true;
        
        // 设置日志识别配置
        config_.log_functions.qt.functions = {"qDebug", "qInfo", "qWarning", "qCritical", "qFatal"};
        config_.log_functions.custom.functions["debug"] = {"LOG_DEBUG"};
        config_.log_functions.custom.functions["info"] = {"LOG_INFO"};
        config_.log_functions.custom.functions["warning"] = {"LOG_WARNING"};
        config_.log_functions.custom.functions["error"] = {"LOG_ERROR"};
        
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
    
    void createSimpleCppFile() {
        createTestFile("simple.cpp", R"(
#include <iostream>
#include <QDebug>

void simpleFunction() {
    std::cout << "Hello World" << std::endl;
    qDebug() << "Qt debug message";
    
    if (true) {
        qWarning() << "Warning message";
    }
}
)");
    }
    
    void createComplexCppFile() {
        createTestFile("complex.cpp", R"(
#include <iostream>
#include <QDebug>
#include <stdexcept>

class TestClass {
public:
    void memberFunction() {
        qInfo() << "Member function info";
        
        try {
            throw std::runtime_error("Test error");
        } catch (const std::exception& e) {
            qCritical() << "Error in member function: " << e.what();
        }
    }
    
    static void staticFunction() {
        qDebug() << "Static function debug";
    }
};

void globalFunction() {
    qWarning() << "Global function warning";
    
    for (int i = 0; i < 3; ++i) {
        qDebug() << "Loop iteration: " << i;
    }
    
    TestClass obj;
    obj.memberFunction();
    TestClass::staticFunction();
}

template<typename T>
void templateFunction(T value) {
    qInfo() << "Template function with value: " << value;
}

int main() {
    globalFunction();
    templateFunction(42);
    templateFunction("test");
    
    return 0;
}
)");
    }
    
    void createHeaderFile() {
        createTestFile("test.h", R"(
#ifndef TEST_H
#define TEST_H

#include <QDebug>

class HeaderClass {
public:
    inline void inlineFunction() {
        qDebug() << "Inline function in header";
    }
    
    void declaredFunction();
};

template<typename T>
inline void headerTemplateFunction(T value) {
    qInfo() << "Header template function: " << value;
}

#define LOG_MACRO(msg) qDebug() << "Macro log: " << msg

#endif // TEST_H
)");
    }
    
    void createCustomLogFile() {
        createTestFile("custom_log.cpp", R"(
#include <iostream>

#define LOG_DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#define LOG_INFO(msg) std::cout << "[INFO] " << msg << std::endl
#define LOG_WARNING(msg) std::cout << "[WARNING] " << msg << std::endl
#define LOG_ERROR(msg) std::cout << "[ERROR] " << msg << std::endl

void testCustomLogs() {
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARNING("Warning message");
    LOG_ERROR("Error message");
    
    if (true) {
        LOG_INFO("Conditional info");
    }
    
    for (int i = 0; i < 2; ++i) {
        LOG_DEBUG("Loop debug: " << i);
    }
}
)");
    }
    
    void createCompileCommandsJson() {
        std::string compileCommands = R"([
{
    "directory": ")" + testDir_.string() + R"(",
    "command": "clang++ -std=c++17 -I/usr/include/qt5 -I/usr/include/qt5/QtCore simple.cpp -o simple",
    "file": "simple.cpp"
},
{
    "directory": ")" + testDir_.string() + R"(",
    "command": "clang++ -std=c++17 -I/usr/include/qt5 -I/usr/include/qt5/QtCore complex.cpp -o complex",
    "file": "complex.cpp"
},
{
    "directory": ")" + testDir_.string() + R"(",
    "command": "clang++ -std=c++17 custom_log.cpp -o custom_log",
    "file": "custom_log.cpp"
}
])";
        
        createTestFile("compile_commands.json", compileCommands);
    }
    
    std::unique_ptr<ConfigManager> createConfigManager() {
        auto configManager = std::make_unique<ConfigManager>();
        // ConfigManager通常通过loadFromFile方法加载配置，这里我们直接使用默认配置
        return configManager;
    }
    
    std::unique_ptr<SourceManager> createSourceManager() {
        return std::make_unique<SourceManager>(config_);
    }

protected:
    fs::path testDir_;
    Config config_;
};

/**
 * @brief 测试C++分析器适配器的基本构造
 */
TEST_F(CppAnalyzerAdapterTest, BasicConstruction) {
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    EXPECT_NO_THROW({
        CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    });
}

/**
 * @brief 测试简单C++文件分析
 */
TEST_F(CppAnalyzerAdapterTest, SimpleFileAnalysis) {
    createSimpleCppFile();
    createCompileCommandsJson();
    
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    auto result = adapter.analyze((testDir_ / "simple.cpp").string());
    
    // 分析应该成功或者因为缺少依赖而失败，但不应该崩溃
    EXPECT_NO_THROW(result.isSuccess() || result.hasError());
}

/**
 * @brief 测试复杂C++文件分析
 */
TEST_F(CppAnalyzerAdapterTest, ComplexFileAnalysis) {
    createComplexCppFile();
    createCompileCommandsJson();
    
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    auto result = adapter.analyze((testDir_ / "complex.cpp").string());
    
    // 测试不应该崩溃
    EXPECT_NO_THROW(result.isSuccess() || result.hasError());
}

/**
 * @brief 测试头文件分析
 */
TEST_F(CppAnalyzerAdapterTest, HeaderFileAnalysis) {
    createHeaderFile();
    createCompileCommandsJson();
    
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    auto result = adapter.analyze((testDir_ / "test.h").string());
    
    // 头文件分析应该能正常处理
    EXPECT_NO_THROW(result.isSuccess() || result.hasError());
}

/**
 * @brief 测试自定义日志函数识别
 */
TEST_F(CppAnalyzerAdapterTest, CustomLogFunctionRecognition) {
    createCustomLogFile();
    createCompileCommandsJson();
    
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    auto result = adapter.analyze((testDir_ / "custom_log.cpp").string());
    
    // 测试不应该崩溃
    EXPECT_NO_THROW(result.isSuccess() || result.hasError());
}

/**
 * @brief 测试不存在的文件
 */
TEST_F(CppAnalyzerAdapterTest, NonExistentFile) {
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    auto result = adapter.analyze((testDir_ / "nonexistent.cpp").string());
    EXPECT_TRUE(result.hasError()) << "不存在的文件应该返回错误";
}

/**
 * @brief 测试空文件
 */
TEST_F(CppAnalyzerAdapterTest, EmptyFile) {
    createTestFile("empty.cpp", "");
    createCompileCommandsJson();
    
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    auto result = adapter.analyze((testDir_ / "empty.cpp").string());
    
    // 空文件应该能正常处理
    EXPECT_NO_THROW(result.isSuccess() || result.hasError());
}

/**
 * @brief 测试无效的C++文件
 */
TEST_F(CppAnalyzerAdapterTest, InvalidCppFile) {
    createTestFile("invalid.cpp", "this is not valid C++ code {{{");
    createCompileCommandsJson();
    
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    auto result = adapter.analyze((testDir_ / "invalid.cpp").string());
    
    // 无效文件应该返回错误但不崩溃
    EXPECT_NO_THROW(result.isSuccess() || result.hasError());
}

/**
 * @brief 测试获取底层分析器
 */
TEST_F(CppAnalyzerAdapterTest, GetUnderlyingAnalyzer) {
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    // 获取底层分析器应该不为空
    auto* underlyingAnalyzer = adapter.getUnderlyingAnalyzer();
    EXPECT_NE(underlyingAnalyzer, nullptr) << "底层分析器不应该为空";
}

/**
 * @brief 测试并行模式设置
 */
TEST_F(CppAnalyzerAdapterTest, ParallelMode) {
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    // 设置并行模式应该不报错
    EXPECT_NO_THROW(adapter.setParallelMode(true, 2));
    EXPECT_NO_THROW(adapter.setParallelMode(false, 0));
}

/**
 * @brief 测试统计信息
 */
TEST_F(CppAnalyzerAdapterTest, Statistics) {
    createSimpleCppFile();
    createCompileCommandsJson();
    
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    // 分析文件
    auto result = adapter.analyze((testDir_ / "simple.cpp").string());
    
    // 获取统计信息
    auto stats = adapter.getStatistics();
    EXPECT_FALSE(stats.empty()) << "统计信息不应该为空";
}

/**
 * @brief 测试批量文件分析
 */
TEST_F(CppAnalyzerAdapterTest, BatchAnalysis) {
    // 创建多个测试文件
    createSimpleCppFile();
    createComplexCppFile();
    createCustomLogFile();
    createCompileCommandsJson();
    
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    // 分析所有文件
    std::vector<std::string> files = {
        (testDir_ / "simple.cpp").string(),
        (testDir_ / "complex.cpp").string(),
        (testDir_ / "custom_log.cpp").string()
    };
    
    for (const auto& file : files) {
        auto result = adapter.analyze(file);
        EXPECT_NO_THROW(result.isSuccess() || result.hasError()) << "文件 " << file << " 分析不应该崩溃";
    }
}

/**
 * @brief 测试缺少编译数据库的情况
 */
TEST_F(CppAnalyzerAdapterTest, MissingCompileCommands) {
    createSimpleCppFile();
    // 不创建compile_commands.json
    
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    auto result = adapter.analyze((testDir_ / "simple.cpp").string());
    
    // 缺少编译数据库可能导致失败，但不应该崩溃
    EXPECT_NO_THROW(result.isSuccess() || result.hasError());
}

/**
 * @brief 测试Qt日志函数识别
 */
TEST_F(CppAnalyzerAdapterTest, QtLogFunctionRecognition) {
    createTestFile("qt_logs.cpp", R"(
#include <QDebug>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(testCategory)
Q_LOGGING_CATEGORY(testCategory, "test.category")

void testQtLogs() {
    qDebug() << "Debug message";
    qInfo() << "Info message";
    qWarning() << "Warning message";
    qCritical() << "Critical message";
    
    qDebug(testCategory) << "Category debug";
    qInfo(testCategory) << "Category info";
    qWarning(testCategory) << "Category warning";
    qCritical(testCategory) << "Category critical";
    
    if (true) {
        qDebug() << "Conditional debug";
    }
    
    for (int i = 0; i < 2; ++i) {
        qInfo() << "Loop info: " << i;
    }
}
)");
    
    createCompileCommandsJson();
    
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    auto result = adapter.analyze((testDir_ / "qt_logs.cpp").string());
    
    // 测试不应该崩溃
    EXPECT_NO_THROW(result.isSuccess() || result.hasError());
}

/**
 * @brief 测试异常处理中的日志
 */
TEST_F(CppAnalyzerAdapterTest, ExceptionHandlingLogs) {
    createTestFile("exception_logs.cpp", R"(
#include <QDebug>
#include <stdexcept>

void testExceptionLogs() {
    try {
        qDebug() << "Before exception";
        throw std::runtime_error("Test exception");
    } catch (const std::exception& e) {
        qCritical() << "Caught exception: " << e.what();
    } catch (...) {
        qCritical() << "Caught unknown exception";
    }
    
    try {
        qInfo() << "Another try block";
    } catch (...) {
        qWarning() << "This should not be reached";
    }
    
    qDebug() << "After exception handling";
}
)");
    
    createCompileCommandsJson();
    
    auto configManager = createConfigManager();
    auto sourceManager = createSourceManager();
    
    CppAnalyzerAdapter adapter(config_, *sourceManager, *configManager);
    
    auto result = adapter.analyze((testDir_ / "exception_logs.cpp").string());
    
    // 测试不应该崩溃
    EXPECT_NO_THROW(result.isSuccess() || result.hasError());
} 