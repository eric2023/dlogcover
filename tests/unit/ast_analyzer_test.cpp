/**
 * @file ast_analyzer_test.cpp
 * @brief AST分析器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/utils/log_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace core {
namespace ast_analyzer {
namespace test {

// 创建测试目录和文件的助手函数
class ASTAnalyzerTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化日志系统，设置为ERROR级别以减少测试期间的日志输出
        utils::Logger::init("", false, utils::LogLevel::ERROR);
        
        // 创建测试目录
        testDir_ = "/tmp/dlogcover_ast_test";
        utils::FileUtils::createDirectory(testDir_);

        // 创建测试文件
        createTestFile(testDir_ + "/test.cpp", R"(
#include <iostream>

// 简单函数
void test_function() {
    std::cout << "测试函数" << std::endl;
}

// 带条件分支的函数
int conditional_function(int value) {
    if (value > 0) {
        std::cout << "正数: " << value << std::endl;
        return value * 2;
    } else {
        std::cerr << "负数: " << value << std::endl;
        return value * -1;
    }
}

// 带循环的函数
void loop_function(int count) {
    // for循环
    for (int i = 0; i < count; ++i) {
        std::cout << "for循环: " << i << std::endl;
    }

    // while循环
    int j = 0;
    while (j < count) {
        std::cout << "while循环: " << j << std::endl;
        ++j;
    }

    // do-while循环
    int k = 0;
    do {
        std::cout << "do-while循环: " << k << std::endl;
        ++k;
    } while (k < count);
}

// 带switch语句的函数
void switch_function(int value) {
    switch (value) {
        case 1:
            std::cout << "选项1" << std::endl;
            break;
        case 2:
            std::cout << "选项2" << std::endl;
            break;
        default:
            std::cout << "默认选项" << std::endl;
            break;
    }
}

// 带异常处理的函数
void exception_function() {
    try {
        throw std::runtime_error("测试异常");
    } catch (const std::exception& e) {
        std::cerr << "捕获异常: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "捕获未知异常" << std::endl;
    }
}

int main() {
    test_function();
    conditional_function(10);
    loop_function(3);
    switch_function(1);
    exception_function();
    return 0;
}
)");

        // 创建带Qt日志的测试文件
        createTestFile(testDir_ + "/qt_log_test.cpp", R"(
#include <QDebug>
#include <QString>

void qt_log_function() {
    qDebug() << "调试信息";
    qInfo() << "普通信息";
    qWarning() << "警告信息";
    qCritical() << "严重错误";
}

void conditional_log() {
    bool condition = true;
    if (condition) {
        qDebug() << "条件为真";
    } else {
        qWarning() << "条件为假";
    }
}

void loop_log() {
    for (int i = 0; i < 3; ++i) {
        qDebug() << "循环次数:" << i;
    }
}

void exception_log() {
    try {
        throw std::runtime_error("错误");
    } catch (const std::exception& e) {
        qCritical() << "异常:" << e.what();
    }
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

        // 创建配置管理器
        configManager_ = std::make_unique<config::ConfigManager>();
        
        // 创建AST分析器
        astAnalyzer_ = std::make_unique<ASTAnalyzer>(config_, *sourceManager_, *configManager_);
    }

    void TearDown() override {
        // 关闭日志系统，确保所有资源正确释放
        utils::Logger::shutdown();
        
        // 清理测试目录
        if (std::filesystem::exists(testDir_)) {
            std::filesystem::remove_all(testDir_);
        }
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

        // 设置日志函数
        config.log_functions.qt.enabled = true;
        config.log_functions.qt.functions = {"qDebug", "qInfo", "qWarning", "qCritical", "qFatal"};

        // 注释掉编译参数和Qt项目标志，因为新配置结构中没有这些字段
        // config.scan.compilerArgs = {"-I/usr/include", "-I/usr/include/c++/8", "-I/usr/include/x86_64-linux-gnu/c++/8",
        //                             "-I/usr/include/x86_64-linux-gnu", "-I/usr/local/include",
        //                             // Qt头文件路径
        //                             "-I/usr/include/x86_64-linux-gnu/qt5", "-I/usr/include/x86_64-linux-gnu/qt5/QtCore",
        //                             "-I/usr/include/x86_64-linux-gnu/qt5/QtGui",
        //                             "-I/usr/include/x86_64-linux-gnu/qt5/QtWidgets",
        //                             // 系统定义
        //                             "-D__GNUG__", "-D__linux__", "-D__x86_64__"};

        // 注释掉Qt项目标志，因为新配置结构中没有这个字段
        // config.scan.isQtProject = true;

        return config;
    }

    std::string testDir_;
    config::Config config_;
    std::unique_ptr<config::ConfigManager> configManager_;
    std::unique_ptr<source_manager::SourceManager> sourceManager_;
    std::unique_ptr<ASTAnalyzer> astAnalyzer_;
};

// 测试初始化和销毁
TEST_F(ASTAnalyzerTestFixture, InitializeAndDestroy) {
    SUCCEED();
}

// 测试分析单个文件
TEST_F(ASTAnalyzerTestFixture, AnalyzeSingleFile) {
    std::string testFilePath = testDir_ + "/test.cpp";

    auto analyzeResult = astAnalyzer_->analyze(testFilePath);
    EXPECT_FALSE(analyzeResult.hasError()) << "分析文件失败: " << analyzeResult.errorMessage();
    EXPECT_TRUE(analyzeResult.value()) << "分析文件返回false";

    const auto* nodeInfo = astAnalyzer_->getASTNodeInfo(testFilePath);
    ASSERT_NE(nullptr, nodeInfo) << "未生成AST节点信息";

    // 验证根节点 - 使用FUNCTION类型而非TRANSLATION_UNIT
    EXPECT_EQ(NodeType::FUNCTION, nodeInfo->type);
    EXPECT_FALSE(nodeInfo->hasLogging);  // 标准输出不算作日志
    EXPECT_GT(nodeInfo->children.size(), 0) << "根节点应该有子节点";
}

// 测试分析所有文件
TEST_F(ASTAnalyzerTestFixture, AnalyzeAllFiles) {
    auto analyzeAllResult = astAnalyzer_->analyzeAll();
    EXPECT_FALSE(analyzeAllResult.hasError()) << "分析所有文件失败: " << analyzeAllResult.errorMessage();
    EXPECT_TRUE(analyzeAllResult.value()) << "分析所有文件返回false";

    const auto& allASTNodeInfo = astAnalyzer_->getAllASTNodeInfo();
    EXPECT_EQ(2, allASTNodeInfo.size()) << "分析文件数量不符";

    std::string testFilePath = testDir_ + "/test.cpp";
    std::string qtTestFilePath = testDir_ + "/qt_log_test.cpp";

    EXPECT_NE(allASTNodeInfo.find(testFilePath), allASTNodeInfo.end()) << "未找到test.cpp的AST节点";
    EXPECT_NE(allASTNodeInfo.find(qtTestFilePath), allASTNodeInfo.end()) << "未找到qt_log_test.cpp的AST节点";
}

// 测试条件语句分析
TEST_F(ASTAnalyzerTestFixture, AnalyzeConditionalStatements) {
    std::string testFilePath = testDir_ + "/test.cpp";
    auto analyzeResult = astAnalyzer_->analyze(testFilePath);
    ASSERT_FALSE(analyzeResult.hasError()) << "分析文件失败: " << analyzeResult.errorMessage();
    ASSERT_TRUE(analyzeResult.value());

    const auto* nodeInfo = astAnalyzer_->getASTNodeInfo(testFilePath);
    ASSERT_NE(nullptr, nodeInfo);

    // 查找conditional_function
    bool foundConditionalFunc = false;
    for (const auto& child : nodeInfo->children) {
        if (child->name == "conditional_function") {
            foundConditionalFunc = true;
            // 验证if语句
            bool foundIfStmt = false;
            for (const auto& stmt : child->children) {
                if (stmt->type == NodeType::IF_STMT) {
                    foundIfStmt = true;
                    EXPECT_EQ(2, stmt->children.size()) << "if语句应该有then和else两个分支";
                    break;
                }
            }
            EXPECT_TRUE(foundIfStmt) << "未找到if语句";
            break;
        }
    }
    EXPECT_TRUE(foundConditionalFunc) << "未找到conditional_function";
}

// 测试循环语句分析
TEST_F(ASTAnalyzerTestFixture, AnalyzeLoopStatements) {
    std::string testFilePath = testDir_ + "/test.cpp";
    auto analyzeResult = astAnalyzer_->analyze(testFilePath);
    ASSERT_FALSE(analyzeResult.hasError()) << "分析文件失败: " << analyzeResult.errorMessage();
    ASSERT_TRUE(analyzeResult.value());

    const auto* nodeInfo = astAnalyzer_->getASTNodeInfo(testFilePath);
    ASSERT_NE(nullptr, nodeInfo);

    // 查找loop_function
    bool foundLoopFunc = false;
    for (const auto& child : nodeInfo->children) {
        if (child->name == "loop_function") {
            foundLoopFunc = true;
            bool foundForStmt = false;
            bool foundWhileStmt = false;
            bool foundDoStmt = false;

            for (const auto& stmt : child->children) {
                if (stmt->type == NodeType::FOR_STMT)
                    foundForStmt = true;
                if (stmt->type == NodeType::WHILE_STMT)
                    foundWhileStmt = true;
                if (stmt->type == NodeType::DO_STMT)
                    foundDoStmt = true;
            }

            EXPECT_TRUE(foundForStmt) << "未找到for循环";
            EXPECT_TRUE(foundWhileStmt) << "未找到while循环";
            EXPECT_TRUE(foundDoStmt) << "未找到do-while循环";
            break;
        }
    }
    EXPECT_TRUE(foundLoopFunc) << "未找到loop_function";
}

// 测试异常处理语句分析
TEST_F(ASTAnalyzerTestFixture, AnalyzeExceptionHandling) {
    std::string testFilePath = testDir_ + "/test.cpp";
    auto analyzeResult = astAnalyzer_->analyze(testFilePath);
    ASSERT_FALSE(analyzeResult.hasError()) << "分析文件失败: " << analyzeResult.errorMessage();
    ASSERT_TRUE(analyzeResult.value());

    const auto* nodeInfo = astAnalyzer_->getASTNodeInfo(testFilePath);
    ASSERT_NE(nullptr, nodeInfo);

    // 查找exception_function
    bool foundExceptionFunc = false;
    for (const auto& child : nodeInfo->children) {
        if (child->name == "exception_function") {
            foundExceptionFunc = true;
            bool foundTryStmt = false;
            bool foundCatchStmt = false;

            for (const auto& stmt : child->children) {
                if (stmt->type == NodeType::TRY_STMT) {
                    foundTryStmt = true;
                    // 验证catch语句是try的子节点
                    for (const auto& catchStmt : stmt->children) {
                        if (catchStmt->type == NodeType::CATCH_STMT) {
                            foundCatchStmt = true;
                            break;
                        }
                    }
                }
            }

            EXPECT_TRUE(foundTryStmt) << "未找到try语句";
            EXPECT_TRUE(foundCatchStmt) << "未找到catch语句";
            break;
        }
    }
    EXPECT_TRUE(foundExceptionFunc) << "未找到exception_function";
}

// 测试Qt日志函数识别
TEST_F(ASTAnalyzerTestFixture, AnalyzeQtLogging) {
    std::string qtTestFilePath = testDir_ + "/qt_log_test.cpp";
    auto analyzeResult = astAnalyzer_->analyze(qtTestFilePath);
    ASSERT_FALSE(analyzeResult.hasError()) << "分析文件失败: " << analyzeResult.errorMessage();
    ASSERT_TRUE(analyzeResult.value());

    const auto* nodeInfo = astAnalyzer_->getASTNodeInfo(qtTestFilePath);
    ASSERT_NE(nullptr, nodeInfo);

    // 查找qt_log_function
    bool foundQtLogFunc = false;
    for (const auto& child : nodeInfo->children) {
        if (child->name == "qt_log_function") {
            foundQtLogFunc = true;
            EXPECT_TRUE(child->hasLogging) << "未识别出Qt日志调用";

            // 验证是否识别出所有日志调用
            int logCallCount = 0;
            for (const auto& stmt : child->children) {
                if (stmt->type == NodeType::LOG_CALL_EXPR) {
                    logCallCount++;
                }
            }
            EXPECT_EQ(4, logCallCount) << "Qt日志调用数量不符";
            break;
        }
    }
    EXPECT_TRUE(foundQtLogFunc) << "未找到qt_log_function";
}

// 测试条件分支中的日志识别
TEST_F(ASTAnalyzerTestFixture, AnalyzeConditionalLogging) {
    std::string qtTestFilePath = testDir_ + "/qt_log_test.cpp";
    auto analyzeResult = astAnalyzer_->analyze(qtTestFilePath);
    ASSERT_FALSE(analyzeResult.hasError()) << "分析文件失败: " << analyzeResult.errorMessage();
    ASSERT_TRUE(analyzeResult.value());

    const auto* nodeInfo = astAnalyzer_->getASTNodeInfo(qtTestFilePath);
    ASSERT_NE(nullptr, nodeInfo);

    // 查找conditional_log
    bool foundCondLogFunc = false;
    for (const auto& child : nodeInfo->children) {
        if (child->name == "conditional_log") {
            foundCondLogFunc = true;
            EXPECT_TRUE(child->hasLogging) << "未识别出条件分支中的日志调用";

            // 验证if语句中的日志
            for (const auto& stmt : child->children) {
                if (stmt->type == NodeType::IF_STMT) {
                    EXPECT_TRUE(stmt->hasLogging) << "if语句中未识别出日志调用";
                    // 验证两个分支都有日志
                    EXPECT_EQ(2, stmt->children.size()) << "if语句应该有两个分支";
                    if (stmt->children.size() == 2) {
                        EXPECT_TRUE(stmt->children[0]->hasLogging) << "then分支未识别出日志调用";
                        EXPECT_TRUE(stmt->children[1]->hasLogging) << "else分支未识别出日志调用";
                    }
                    break;
                }
            }
            break;
        }
    }
    EXPECT_TRUE(foundCondLogFunc) << "未找到conditional_log函数";
}

// 测试AST单元管理
TEST_F(ASTAnalyzerTestFixture, ASTUnitManagement) {
    std::string testFilePath = testDir_ + "/test.cpp";
    std::string qtTestFilePath = testDir_ + "/qt_log_test.cpp";

    auto analyzeResult1 = astAnalyzer_->analyze(testFilePath);
    ASSERT_FALSE(analyzeResult1.hasError()) << "分析文件失败: " << analyzeResult1.errorMessage();
    ASSERT_TRUE(analyzeResult1.value());

    auto analyzeResult2 = astAnalyzer_->analyze(qtTestFilePath);
    ASSERT_FALSE(analyzeResult2.hasError()) << "分析文件失败: " << analyzeResult2.errorMessage();
    ASSERT_TRUE(analyzeResult2.value());

    // 验证节点信息是否正确
    const auto* nodeInfo = astAnalyzer_->getASTNodeInfo(testFilePath);
    ASSERT_NE(nullptr, nodeInfo);
    const auto* qtNodeInfo = astAnalyzer_->getASTNodeInfo(qtTestFilePath);
    ASSERT_NE(nullptr, qtNodeInfo);
}

// 测试上下文管理
TEST_F(ASTAnalyzerTestFixture, ContextManagement) {
    std::string testFilePath = testDir_ + "/test.cpp";
    auto analyzeResult = astAnalyzer_->analyze(testFilePath);
    ASSERT_FALSE(analyzeResult.hasError()) << "分析文件失败: " << analyzeResult.errorMessage();
    ASSERT_TRUE(analyzeResult.value());

    // 验证节点信息
    const auto* nodeInfo = astAnalyzer_->getASTNodeInfo(testFilePath);
    ASSERT_NE(nullptr, nodeInfo);

    // 查找conditional_function并验证其结构
    bool foundConditionalFunc = false;
    for (const auto& child : nodeInfo->children) {
        if (child->name == "conditional_function") {
            foundConditionalFunc = true;
            EXPECT_TRUE(child->hasLogging) << "conditional_function应该包含日志调用";
            break;
        }
    }
    EXPECT_TRUE(foundConditionalFunc) << "未找到conditional_function";
}

// 测试嵌套上下文管理
TEST_F(ASTAnalyzerTestFixture, NestedContextManagement) {
    std::string qtTestFilePath = testDir_ + "/qt_log_test.cpp";
    auto analyzeResult = astAnalyzer_->analyze(qtTestFilePath);
    ASSERT_FALSE(analyzeResult.hasError()) << "分析文件失败: " << analyzeResult.errorMessage();
    ASSERT_TRUE(analyzeResult.value());

    const auto* nodeInfo = astAnalyzer_->getASTNodeInfo(qtTestFilePath);
    ASSERT_NE(nullptr, nodeInfo);

    // 查找conditional_log函数并验证其嵌套结构
    bool foundConditionalLog = false;
    for (const auto& child : nodeInfo->children) {
        if (child->name == "conditional_log") {
            foundConditionalLog = true;
            EXPECT_TRUE(child->hasLogging) << "conditional_log应该包含日志调用";

            // 查找if语句并验证其结构
            bool foundIfStmt = false;
            for (const auto& stmt : child->children) {
                if (stmt->type == NodeType::IF_STMT) {
                    foundIfStmt = true;
                    EXPECT_TRUE(stmt->hasLogging) << "if语句应该包含日志调用";

                    // 验证then和else分支
                    ASSERT_EQ(2, stmt->children.size());
                    EXPECT_TRUE(stmt->children[0]->hasLogging) << "then分支应该包含日志调用";
                    EXPECT_TRUE(stmt->children[1]->hasLogging) << "else分支应该包含日志调用";
                    break;
                }
            }
            EXPECT_TRUE(foundIfStmt) << "未找到if语句";
            break;
        }
    }
    EXPECT_TRUE(foundConditionalLog) << "未找到conditional_log函数";
}

// 测试错误处理和边界条件
TEST_F(ASTAnalyzerTestFixture, ErrorHandlingAndBoundaryConditions) {
    // 测试空文件路径
    auto emptyResult = astAnalyzer_->analyze("");
    EXPECT_TRUE(emptyResult.hasError()) << "空文件路径应该导致错误";

    // 测试不存在的文件
    auto nonexistentResult = astAnalyzer_->analyze("/path/to/nonexistent/file.cpp");
    EXPECT_TRUE(nonexistentResult.hasError()) << "不存在的文件应该导致错误";

    // 测试无效的文件内容
    std::string invalidFilePath = testDir_ + "/invalid.cpp";
    createTestFile(invalidFilePath, "invalid c++ code");
    auto invalidResult = astAnalyzer_->analyze(invalidFilePath);
    EXPECT_TRUE(invalidResult.hasError()) << "无效的文件内容应该导致错误";

    // 测试空目录分析
    config_.scan.directories.clear();
    auto emptyDirResult = astAnalyzer_->analyzeAll();
    EXPECT_TRUE(emptyDirResult.hasError()) << "空目录应该导致错误";

    // 测试重复分析同一文件
    std::string testFilePath = testDir_ + "/test.cpp";
    auto firstResult = astAnalyzer_->analyze(testFilePath);
    EXPECT_FALSE(firstResult.hasError()) << "首次分析文件失败: " << firstResult.errorMessage();
    EXPECT_TRUE(firstResult.value());
    const auto* firstAnalysis = astAnalyzer_->getASTNodeInfo(testFilePath);

    auto secondResult = astAnalyzer_->analyze(testFilePath);
    EXPECT_FALSE(secondResult.hasError()) << "重复分析文件失败: " << secondResult.errorMessage();
    EXPECT_TRUE(secondResult.value());
    const auto* secondAnalysis = astAnalyzer_->getASTNodeInfo(testFilePath);

    EXPECT_NE(nullptr, firstAnalysis);
    EXPECT_NE(nullptr, secondAnalysis);
}

}  // namespace test
}  // namespace ast_analyzer
}  // namespace core
}  // namespace dlogcover
