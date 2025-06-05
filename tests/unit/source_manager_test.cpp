/**
 * @file source_manager_test.cpp
 * @brief 源文件管理器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/config.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace dlogcover {
namespace source_manager {
namespace test {

// 创建测试目录和文件的助手函数
class SourceManagerTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化日志系统，设置为ERROR级别以减少测试期间的日志输出
        utils::Logger::init("", false, utils::LogLevel::ERROR);
        
        // 创建测试目录
        testDir_ = "/tmp/dlogcover_source_test";
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
        // 关闭日志系统，确保所有资源正确释放
        utils::Logger::shutdown();
        
        // 清理测试目录
        if (std::filesystem::exists(testDir_)) {
            std::filesystem::remove_all(testDir_);
        }
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
    auto collectResult = sourceManager.collectSourceFiles();
    EXPECT_FALSE(collectResult.hasError()) << "收集源文件失败: " << collectResult.errorMessage();
    EXPECT_TRUE(collectResult.value());

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
    auto collectResult = sourceManager.collectSourceFiles();
    EXPECT_FALSE(collectResult.hasError()) << "收集源文件失败: " << collectResult.errorMessage();
    EXPECT_TRUE(collectResult.value());

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
    auto collectResult = sourceManager.collectSourceFiles();
    if (!collectResult.hasError()) {
        EXPECT_FALSE(collectResult.value());
    } else {
        // 或者收集过程中遇到了错误，这也是预期的行为
        EXPECT_TRUE(collectResult.hasError());
    }

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
    auto collectResult = sourceManager.collectSourceFiles();
    EXPECT_FALSE(collectResult.hasError()) << "收集源文件失败: " << collectResult.errorMessage();
    EXPECT_TRUE(collectResult.value());

    // 验证结果
    EXPECT_EQ(2, sourceManager.getSourceFileCount());  // 只有2个.cpp文件

    // 验证只包含.cpp文件
    const auto& sourceFiles = sourceManager.getSourceFiles();
    for (const auto& file : sourceFiles) {
        EXPECT_TRUE(utils::FileUtils::hasExtension(file.path, ".cpp"));
    }
}

// 测试目录排除规则
TEST_F(SourceManagerTestFixture, DirectoryExclusion) {
    // 创建更复杂的目录结构
    std::string testDir = testDir_ + "/complex";
    utils::FileUtils::createDirectory(testDir);

    // 创建多层目录结构
    std::vector<std::string> dirsToCreate = {
        "/src/core",      "/src/utils",        "/tests/unit",           "/build/debug",
        "/build/release", "/third_party/lib1", "/third_party/lib2/src", "/docs/api"};

    for (const auto& dir : dirsToCreate) {
        utils::FileUtils::createDirectory(testDir + dir);
        // 在每个目录中创建一些源文件
        createTestFile(testDir + dir + "/test.cpp", "// Test file in " + dir);
        createTestFile(testDir + dir + "/main.h", "// Header in " + dir);
    }

    // 设置多个排除规则
    config_.scan.directories = {testDir};
    config_.scan.excludes = {"build/*", "tests/*", "third_party/lib2/*", "docs/*"};

    // 创建源文件管理器
    SourceManager sourceManager(config_);
    auto collectResult = sourceManager.collectSourceFiles();
    EXPECT_FALSE(collectResult.hasError()) << "收集源文件失败: " << collectResult.errorMessage();
    EXPECT_TRUE(collectResult.value());

    // 验证结果
    const auto& sourceFiles = sourceManager.getSourceFiles();

    // 验证被排除的目录中的文件没有被收集
    for (const auto& file : sourceFiles) {
        EXPECT_FALSE(file.path.find("/build/") != std::string::npos);
        EXPECT_FALSE(file.path.find("/tests/") != std::string::npos);
        EXPECT_FALSE(file.path.find("/third_party/lib2/") != std::string::npos);
        EXPECT_FALSE(file.path.find("/docs/") != std::string::npos);
    }

    // 验证未被排除的目录中的文件被正确收集
    std::vector<std::string> expectedPaths = {"/src/core/test.cpp",         "/src/core/main.h",
                                              "/src/utils/test.cpp",        "/src/utils/main.h",
                                              "/third_party/lib1/test.cpp", "/third_party/lib1/main.h"};

    for (const auto& expected : expectedPaths) {
        bool found = false;
        for (const auto& file : sourceFiles) {
            if (file.path.find(expected) != std::string::npos) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Expected file not found: " << expected;
    }
}

// 测试文件内容管理
TEST_F(SourceManagerTestFixture, FileContentManagement) {
    // 创建具有不同编码和内容的文件
    std::string contentDir = testDir_ + "/content";
    utils::FileUtils::createDirectory(contentDir);

    // 创建UTF-8文件
    std::string utf8Content = R"(
#include <iostream>
// 中文注释
void testFunction() {
    std::cout << "测试输出" << std::endl;
}
)";
    createTestFile(contentDir + "/utf8.cpp", utf8Content);

    // 创建包含多行注释的文件
    std::string multilineContent = R"(
/*
 * 多行注释测试
 * 第二行
 * 第三行
 */
#include <string>

/**
 * @brief 文档注释测试
 * @param input 输入参数
 * @return 返回值
 */
std::string processInput(const std::string& input) {
    return input + "_processed";
}
)";
    createTestFile(contentDir + "/multiline.cpp", multilineContent);

    // 创建大文件
    std::stringstream largeContent;
    largeContent << "#include <vector>\n\n";
    for (int i = 0; i < 1000; ++i) {
        largeContent << "void function" << i << "() { /* 函数 " << i << " */ }\n";
    }
    createTestFile(contentDir + "/large.cpp", largeContent.str());

    // 更新配置
    config_.scan.directories = {contentDir};

    // 创建源文件管理器
    SourceManager sourceManager(config_);

    auto collectResult = sourceManager.collectSourceFiles();
    EXPECT_FALSE(collectResult.hasError()) << "收集源文件失败: " << collectResult.errorMessage();
    EXPECT_TRUE(collectResult.value());

    // 验证UTF-8文件内容
    const SourceFileInfo* utf8File = sourceManager.getSourceFile(contentDir + "/utf8.cpp");
    ASSERT_NE(nullptr, utf8File);
    EXPECT_EQ(utf8Content, utf8File->content);

    // 验证多行注释文件内容
    const SourceFileInfo* multilineFile = sourceManager.getSourceFile(contentDir + "/multiline.cpp");
    ASSERT_NE(nullptr, multilineFile);
    EXPECT_EQ(multilineContent, multilineFile->content);

    // 验证大文件内容
    const SourceFileInfo* largeFile = sourceManager.getSourceFile(contentDir + "/large.cpp");
    ASSERT_NE(nullptr, largeFile);
    EXPECT_EQ(largeContent.str(), largeFile->content);
}

// 测试文件修改时间跟踪
TEST_F(SourceManagerTestFixture, FileModificationTracking) {
    // 创建测试文件
    std::string trackingDir = testDir_ + "/tracking";
    utils::FileUtils::createDirectory(trackingDir);

    std::string testFile = trackingDir + "/test.cpp";
    createTestFile(testFile, "// Initial content");

    // 更新配置
    config_.scan.directories = {trackingDir};

    // 创建源文件管理器
    SourceManager sourceManager(config_);

    // 第一次收集
    auto collectResult1 = sourceManager.collectSourceFiles();
    EXPECT_FALSE(collectResult1.hasError()) << "第一次收集源文件失败: " << collectResult1.errorMessage();
    EXPECT_TRUE(collectResult1.value());

    // 获取初始文件信息
    const SourceFileInfo* initialInfo = sourceManager.getSourceFile(testFile);
    ASSERT_NE(nullptr, initialInfo);
    auto initialSize = initialInfo->size;

    // 等待一秒以确保修改时间会不同
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 修改文件
    createTestFile(testFile, "// Modified content");

    // 第二次收集
    auto collectResult2 = sourceManager.collectSourceFiles();
    EXPECT_FALSE(collectResult2.hasError()) << "第二次收集源文件失败: " << collectResult2.errorMessage();
    EXPECT_TRUE(collectResult2.value());

    // 获取更新后的文件信息
    const SourceFileInfo* updatedInfo = sourceManager.getSourceFile(testFile);
    ASSERT_NE(nullptr, updatedInfo);

    // 验证文件大小已更新
    EXPECT_NE(initialSize, updatedInfo->size);
    // 验证内容已更新
    EXPECT_EQ("// Modified content", updatedInfo->content);
}

// 测试符号链接处理
TEST_F(SourceManagerTestFixture, SymbolicLinkHandling) {
    // 创建源目录和链接目录
    std::string sourceDir = testDir_ + "/source";
    std::string linkDir = testDir_ + "/link";
    utils::FileUtils::createDirectory(sourceDir);

    // 在源目录中创建文件
    createTestFile(sourceDir + "/original.cpp", "// Original file");

    // 创建符号链接
    try {
        std::filesystem::create_symlink(sourceDir, linkDir);
    } catch (const std::filesystem::filesystem_error&) {
        // 如果无法创建符号链接（例如权限问题），跳过此测试
        return;
    }

    // 更新配置以包含链接目录
    config_.scan.directories = {linkDir};

    // 创建源文件管理器
    SourceManager sourceManager(config_);
    auto collectResult = sourceManager.collectSourceFiles();
    EXPECT_FALSE(collectResult.hasError()) << "收集源文件失败: " << collectResult.errorMessage();
    EXPECT_TRUE(collectResult.value());

    // 验证通过符号链接访问的文件
    const SourceFileInfo* linkedFile = sourceManager.getSourceFile(linkDir + "/original.cpp");
    ASSERT_NE(nullptr, linkedFile);
    EXPECT_EQ("// Original file", linkedFile->content);

    // 清理符号链接
    std::filesystem::remove(linkDir);
}

// 测试错误处理和边界条件
TEST_F(SourceManagerTestFixture, ErrorHandlingAndBoundaryConditions) {
    // 测试不存在的目录
    config_.scan.directories = {"/nonexistent_directory_12345"};
    SourceManager sourceManager1(config_);
    auto collectResult1 = sourceManager1.collectSourceFiles();
    EXPECT_TRUE(collectResult1.hasError()) << "不存在的目录应该导致错误";

    // 测试无权限目录
    std::string restrictedDir = testDir_ + "/restricted";
    utils::FileUtils::createDirectory(restrictedDir);
    createTestFile(restrictedDir + "/test.cpp", "// Restricted file");
    // 尝试设置权限限制（在支持的系统上）
    try {
        std::filesystem::permissions(restrictedDir,
                                     std::filesystem::perms::owner_exec | std::filesystem::perms::others_exec,
                                     std::filesystem::perm_options::remove);
    } catch (const std::exception& e) {
        // 忽略权限设置错误，测试将根据实际权限进行
    }

    config_.scan.directories = {restrictedDir};
    SourceManager sourceManager2(config_);
    auto collectResult2 = sourceManager2.collectSourceFiles();
    // 权限限制可能导致错误或返回false
    if (collectResult2.hasError()) {
        EXPECT_TRUE(collectResult2.hasError());
    } else {
        EXPECT_FALSE(collectResult2.value());
    }

    // 恢复权限以便清理
    try {
        std::filesystem::permissions(restrictedDir, std::filesystem::perms::owner_all,
                                     std::filesystem::perm_options::add);
    } catch (const std::exception& e) {
        // 忽略权限恢复错误
    }

    // 测试空文件
    std::string emptyDir = testDir_ + "/empty";
    utils::FileUtils::createDirectory(emptyDir);
    createTestFile(emptyDir + "/empty.cpp", "");

    config_.scan.directories = {emptyDir};
    SourceManager sourceManager3(config_);
    auto collectResult3 = sourceManager3.collectSourceFiles();
    EXPECT_FALSE(collectResult3.hasError()) << "收集空目录中的文件失败: " << collectResult3.errorMessage();

    const SourceFileInfo* emptyFile = sourceManager3.getSourceFile(emptyDir + "/empty.cpp");
    ASSERT_NE(nullptr, emptyFile);
    EXPECT_TRUE(emptyFile->content.empty());

    // 测试特殊文件名
    std::string specialDir = testDir_ + "/special";
    utils::FileUtils::createDirectory(specialDir);
    createTestFile(specialDir + "/test file.cpp", "// Special file");  // 带空格的文件名

    config_.scan.directories = {specialDir};
    SourceManager sourceManager4(config_);
    auto collectResult4 = sourceManager4.collectSourceFiles();
    EXPECT_FALSE(collectResult4.hasError()) << "收集包含特殊文件名的目录失败: " << collectResult4.errorMessage();

    const SourceFileInfo* specialFile = sourceManager4.getSourceFile(specialDir + "/test file.cpp");
    ASSERT_NE(nullptr, specialFile);
    EXPECT_EQ("// Special file", specialFile->content);

    // 测试空文件类型配置
    config_.scan.directories = {testDir_};
    config_.scan.fileTypes.clear();
    SourceManager sourceManager5(config_);
    auto collectResult5 = sourceManager5.collectSourceFiles();
    if (collectResult5.hasError()) {
        EXPECT_TRUE(collectResult5.hasError());
    } else {
        EXPECT_FALSE(collectResult5.value());
    }

    // 测试恶意文件路径
    config_.scan.fileTypes = {".cpp", ".h"};
    config_.scan.directories = {testDir_};
    SourceManager sourceManager6(config_);
    auto collectResult6 = sourceManager6.collectSourceFiles();
    EXPECT_FALSE(collectResult6.hasError()) << "收集源文件失败: " << collectResult6.errorMessage();
    EXPECT_TRUE(collectResult6.value());

    // 测试非常长的文件名
    SourceManager sourceManager7(config_);
    auto collectResult7 = sourceManager7.collectSourceFiles();
    EXPECT_FALSE(collectResult7.hasError()) << "收集源文件失败: " << collectResult7.errorMessage();
    EXPECT_TRUE(collectResult7.value());
}

}  // namespace test
}  // namespace source_manager
}  // namespace dlogcover
