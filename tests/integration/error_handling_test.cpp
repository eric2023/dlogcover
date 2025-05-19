/**
 * @file error_handling_test.cpp
 * @brief 错误处理集成测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/cli/command_line_parser.h>
#include <dlogcover/config/config_manager.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/core/log_identifier/log_identifier.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "../common/test_utils.h"

namespace dlogcover {
namespace test {

class ErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        test_dir_ = TestUtils::createTestTempDir("error_test_");
        ASSERT_FALSE(test_dir_.empty());

        // 初始化日志系统
        log_file_ = test_dir_ + "/test.log";
        utils::Logger::init(log_file_, true, utils::LogLevel::INFO);

        // 创建测试源文件目录
        source_dir_ = test_dir_ + "/src";
        std::filesystem::create_directories(source_dir_);

        // 初始化配置和源文件管理器
        config_ = config::ConfigManager::getDefaultConfig();
        source_manager_ = std::make_unique<source_manager::SourceManager>(config_);
    }

    void TearDown() override {
        // 关闭日志系统
        utils::Logger::shutdown();

        // 清理临时目录
        if (!test_dir_.empty()) {
            TestUtils::cleanupTestTempDir(test_dir_);
        }
    }

    // 创建测试源文件
    std::string createTestSource(const std::string& filename, const std::string& content) {
        std::string file_path = source_dir_ + "/" + filename;
        std::ofstream source_file(file_path);
        EXPECT_TRUE(source_file.is_open());
        source_file << content;
        source_file.close();
        return file_path;
    }

protected:
    std::string test_dir_;
    std::string log_file_;
    std::string source_dir_;
    config::Config config_;
    std::unique_ptr<source_manager::SourceManager> source_manager_;
};

// 测试无效源文件处理
TEST_F(ErrorHandlingTest, InvalidSourceFile) {
    // 创建一个语法错误的源文件
    std::string invalid_source = R"(
        #include <QDebug>

        class InvalidClass {
        public:
            void brokenMethod() {
                qDebug() << "This is broken;  // 缺少引号
                if (true {                    // 缺少括号
                    qInfo() << "Error";
                }
            }
        };
    )";

    std::string source_path = createTestSource("invalid.cpp", invalid_source);

    // 创建分析器
    core::ast_analyzer::ASTAnalyzer ast_analyzer(config_, *source_manager_);
    core::log_identifier::LogIdentifier identifier(config_, ast_analyzer);

    // 分析文件应该失败
    EXPECT_THROW(ast_analyzer.analyze(source_path), std::runtime_error);
    EXPECT_THROW(identifier.identifyLogCalls(), std::runtime_error);
}

// 测试文件权限错误
TEST_F(ErrorHandlingTest, FilePermissionError) {
    // 创建一个只读目录
    std::string readonly_dir = test_dir_ + "/readonly";
    std::filesystem::create_directories(readonly_dir);
    std::filesystem::permissions(readonly_dir, std::filesystem::perms::owner_read | std::filesystem::perms::group_read |
                                                   std::filesystem::perms::others_read);

    // 尝试在只读目录中创建文件
    std::string test_file = readonly_dir + "/test_write.txt";
    std::string content = "Test content";
    EXPECT_FALSE(utils::FileUtils::writeFile(test_file, content));
}

// 测试内存限制处理
TEST_F(ErrorHandlingTest, MemoryLimitHandling) {
    // 创建一个非常大的源文件
    std::stringstream large_source;
    large_source << "#include <QDebug>\n\nclass LargeClass {\npublic:\n";

    // 生成大量的方法
    for (int i = 0; i < 10000; ++i) {
        large_source << "    void method" << i << "() {\n";
        large_source << "        qDebug() << \"Method " << i << "\";\n";
        large_source << "    }\n\n";
    }

    large_source << "};\n";

    std::string source_path = createTestSource("large_file.cpp", large_source.str());

    // 创建分析器
    core::ast_analyzer::ASTAnalyzer ast_analyzer(config_, *source_manager_);
    core::log_identifier::LogIdentifier identifier(config_, ast_analyzer);

    // 分析大文件应该触发内存限制保护
    EXPECT_THROW(ast_analyzer.analyze(source_path), std::runtime_error);
    EXPECT_THROW(identifier.identifyLogCalls(), std::runtime_error);
}

// 测试递归包含处理
TEST_F(ErrorHandlingTest, RecursiveIncludeHandling) {
    // 创建循环包含的头文件
    std::string header1 = R"(
        #pragma once
        #include "header2.h"

        class Class1 {
        public:
            void method1();
        };
    )";

    std::string header2 = R"(
        #pragma once
        #include "header1.h"

        class Class2 {
        public:
            void method2();
        };
    )";

    std::string source = R"(
        #include "header1.h"
        #include <QDebug>

        void Class1::method1() {
            qDebug() << "Method 1";
        }

        void Class2::method2() {
            qDebug() << "Method 2";
        }
    )";

    createTestSource("header1.h", header1);
    createTestSource("header2.h", header2);
    std::string source_path = createTestSource("source.cpp", source);

    // 创建分析器
    core::ast_analyzer::ASTAnalyzer ast_analyzer(config_, *source_manager_);
    core::log_identifier::LogIdentifier identifier(config_, ast_analyzer);

    // 分析循环包含的文件应该被正确处理
    EXPECT_NO_THROW(ast_analyzer.analyze(source_path));
    EXPECT_NO_THROW(identifier.identifyLogCalls());
}

// 测试编码错误处理
TEST_F(ErrorHandlingTest, EncodingErrorHandling) {
    // 创建包含非UTF-8编码字符的源文件
    std::string invalid_encoding = "void test() { qDebug() << \"\xFF\xFF\"; }";
    std::string source_path = createTestSource("invalid_encoding.cpp", invalid_encoding);

    // 创建分析器
    core::ast_analyzer::ASTAnalyzer ast_analyzer(config_, *source_manager_);
    core::log_identifier::LogIdentifier identifier(config_, ast_analyzer);

    // 分析非UTF-8文件应该失败
    EXPECT_THROW(ast_analyzer.analyze(source_path), std::runtime_error);
    EXPECT_THROW(identifier.identifyLogCalls(), std::runtime_error);
}

// 测试并发分析错误处理
TEST_F(ErrorHandlingTest, ConcurrentAnalysisHandling) {
    // 创建多个测试文件
    std::vector<std::string> source_files;
    for (int i = 0; i < 5; ++i) {
        std::string source = R"(
            #include <QDebug>

            void test)" + std::to_string(i) +
                             R"(() {
                qDebug() << "Test )" +
                             std::to_string(i) + R"(";
            }
        )";
        source_files.push_back(createTestSource("test" + std::to_string(i) + ".cpp", source));
    }

    // 并发分析所有文件
    std::vector<std::future<void>> futures;
    for (const auto& file : source_files) {
        futures.push_back(std::async(std::launch::async, [&, file]() {
            // 为每个线程创建单独的分析器
            config::Config thread_config = config::ConfigManager::getDefaultConfig();
            auto thread_source_manager = std::make_unique<source_manager::SourceManager>(thread_config);
            core::ast_analyzer::ASTAnalyzer ast_analyzer(thread_config, *thread_source_manager);
            core::log_identifier::LogIdentifier identifier(thread_config, ast_analyzer);

            EXPECT_NO_THROW(ast_analyzer.analyze(file));
            EXPECT_NO_THROW(identifier.identifyLogCalls());
        }));
    }

    // 等待所有分析完成
    for (auto& future : futures) {
        future.get();
    }
}

}  // namespace test
}  // namespace dlogcover
