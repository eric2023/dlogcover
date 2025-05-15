/**
 * @file source_manager_test.cpp
 * @brief 源文件管理器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace source_manager {
namespace test {

// 创建测试目录和文件的助手函数
class SourceManagerTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试目录
        testDir_ = std::filesystem::temp_directory_path().string() + "/dlogcover_source_test";
        utils::FileUtils::createDirectory(testDir_);

        // 创建测试文件结构
        // src目录
        std::string srcDir = testDir_ + "/src";
        utils::FileUtils::createDirectory(srcDir);

        // include目录
        std::string includeDir = testDir_ + "/include";
        utils::FileUtils::createDirectory(includeDir);

        // build目录（将被排除）
        std::string buildDir = testDir_ + "/build";
        utils::FileUtils::createDirectory(buildDir);

        // 创建测试文件
        createTestFile(srcDir + "/main.cpp", "int main() { return 0; }");
        createTestFile(srcDir + "/utils.cpp", "void utils() {}");
        createTestFile(includeDir + "/header.h", "#pragma once\n");
        createTestFile(buildDir + "/generated.cpp", "// 自动生成的文件");

        // 设置配置
        config_ = createTestConfig();
    }

    void TearDown() override {
        // 删除测试目录和所有内容
        std::filesystem::remove_all(testDir_);
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

        // 设置排除模式
        config.scan.excludes = {"build/"};

        // 设置文件类型
        config.scan.fileTypes = {".cpp", ".h", ".hpp"};

        return config;
    }

    std::string testDir_;
    config::Config config_;
};

// 测试源文件收集
TEST_F(SourceManagerTestFixture, CollectSourceFiles) {
    // 创建源文件管理器
    SourceManager sourceManager(config_);

    // 收集源文件
    EXPECT_TRUE(sourceManager.collectSourceFiles());

    // 验证结果
    EXPECT_EQ(3, sourceManager.getSourceFileCount());  // 3个文件（2个.cpp, 1个.h）

    // 验证文件路径
    const auto& sourceFiles = sourceManager.getSourceFiles();
    std::vector<std::string> expectedRelativePaths = {"src/main.cpp", "src/utils.cpp", "include/header.h"};

    // 收集实际相对路径
    std::vector<std::string> actualRelativePaths;
    for (const auto& file : sourceFiles) {
        std::string relativePath = file.relativePath;
        // 替换Windows风格的路径分隔符为Unix风格
        std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
        actualRelativePaths.push_back(relativePath);
    }

    // 排序以便比较
    std::sort(expectedRelativePaths.begin(), expectedRelativePaths.end());
    std::sort(actualRelativePaths.begin(), actualRelativePaths.end());

    EXPECT_EQ(expectedRelativePaths, actualRelativePaths);

    // 验证build目录中的文件被排除
    for (const auto& file : sourceFiles) {
        EXPECT_TRUE(file.path.find("/build/") == std::string::npos);
    }
}

// 测试获取源文件
TEST_F(SourceManagerTestFixture, GetSourceFile) {
    // 创建源文件管理器
    SourceManager sourceManager(config_);

    // 收集源文件
    EXPECT_TRUE(sourceManager.collectSourceFiles());

    // 测试获取存在的文件
    std::string mainCppPath = testDir_ + "/src/main.cpp";
    const SourceFileInfo* mainFile = sourceManager.getSourceFile(mainCppPath);
    EXPECT_NE(nullptr, mainFile);
    EXPECT_EQ(mainCppPath, mainFile->path);
    EXPECT_EQ("int main() { return 0; }", mainFile->content);
    EXPECT_FALSE(mainFile->isHeader);

    // 测试获取头文件
    std::string headerPath = testDir_ + "/include/header.h";
    const SourceFileInfo* headerFile = sourceManager.getSourceFile(headerPath);
    EXPECT_NE(nullptr, headerFile);
    EXPECT_EQ(headerPath, headerFile->path);
    EXPECT_TRUE(headerFile->isHeader);

    // 测试获取不存在的文件
    EXPECT_EQ(nullptr, sourceManager.getSourceFile("non_existent_file.cpp"));
}

// 测试空配置
TEST_F(SourceManagerTestFixture, EmptyConfig) {
    // 创建空配置
    config::Config emptyConfig;
    emptyConfig.scan.directories.clear();

    // 创建源文件管理器
    SourceManager sourceManager(emptyConfig);

    // 收集源文件应该失败
    EXPECT_FALSE(sourceManager.collectSourceFiles());

    // 验证结果
    EXPECT_EQ(0, sourceManager.getSourceFileCount());
}

// 测试文件类型过滤
TEST_F(SourceManagerTestFixture, FileTypeFiltering) {
    // 修改配置，只包含.cpp文件
    config_.scan.fileTypes = {".cpp"};

    // 创建源文件管理器
    SourceManager sourceManager(config_);

    // 收集源文件
    EXPECT_TRUE(sourceManager.collectSourceFiles());

    // 验证结果
    EXPECT_EQ(2, sourceManager.getSourceFileCount());  // 只有2个.cpp文件

    // 验证只包含.cpp文件
    const auto& sourceFiles = sourceManager.getSourceFiles();
    for (const auto& file : sourceFiles) {
        EXPECT_TRUE(utils::FileUtils::hasExtension(file.path, ".cpp"));
    }
}

}  // namespace test
}  // namespace source_manager
}  // namespace dlogcover
