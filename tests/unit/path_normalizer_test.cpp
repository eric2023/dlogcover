/**
 * @file path_normalizer_test.cpp
 * @brief 路径规范化工具单元测试
 * @author DLogCover Team
 * @date 2024
 */

#include <gtest/gtest.h>
#include "dlogcover/utils/path_normalizer.h"
#include <filesystem>
#include <fstream>

using namespace dlogcover::utils;

class PathNormalizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试目录结构
        testDir = std::filesystem::temp_directory_path() / "path_normalizer_test";
        std::filesystem::create_directories(testDir);
        std::filesystem::create_directories(testDir / "subdir");
        
        // 创建测试文件
        createTestFile(testDir / "test.txt", "test content");
        createTestFile(testDir / "subdir" / "nested.txt", "nested content");
        
        // 保存当前工作目录
        originalCwd = std::filesystem::current_path();
    }
    
    void TearDown() override {
        // 恢复工作目录
        std::filesystem::current_path(originalCwd);
        
        // 清理测试文件
        std::filesystem::remove_all(testDir);
    }
    
    void createTestFile(const std::filesystem::path& path, const std::string& content) {
        std::ofstream file(path);
        file << content;
        file.close();
    }
    
    std::filesystem::path testDir;
    std::filesystem::path originalCwd;
};

// 测试路径规范化
TEST_F(PathNormalizerTest, NormalizePath) {
    // 测试基本规范化
    std::string path1 = "/home/user/../user/documents/./file.txt";
    std::string normalized1 = PathNormalizer::normalize(path1);
    EXPECT_EQ(normalized1, "/home/user/documents/file.txt");
    
    // 测试Windows风格路径
    std::string path2 = "C:\\Users\\..\\Users\\Documents\\.\\file.txt";
    std::string normalized2 = PathNormalizer::normalize(path2);
    // 结果可能因平台而异，但应该移除 .. 和 .
    EXPECT_TRUE(normalized2.find("..") == std::string::npos);
    EXPECT_TRUE(normalized2.find("/.") == std::string::npos);
    
    // 测试空路径
    EXPECT_EQ(PathNormalizer::normalize(""), "");
    
    // 测试相对路径
    std::string path3 = "./src/../include/header.h";
    std::string normalized3 = PathNormalizer::normalize(path3);
    EXPECT_EQ(normalized3, "include/header.h");
}

// 测试获取规范路径
TEST_F(PathNormalizerTest, GetCanonicalPath) {
    std::string testFile = (testDir / "test.txt").string();
    std::string relativeFile = (testDir / "subdir" / ".." / "test.txt").string();
    
    std::string canonical1 = PathNormalizer::getCanonicalPath(testFile);
    std::string canonical2 = PathNormalizer::getCanonicalPath(relativeFile);
    
    // 两个路径应该规范化为相同的绝对路径
    EXPECT_EQ(canonical1, canonical2);
    EXPECT_TRUE(PathNormalizer::isAbsolutePath(canonical1));
    
    // 测试不存在的文件
    std::string nonExistent = (testDir / "nonexistent.txt").string();
    std::string canonicalNonExistent = PathNormalizer::getCanonicalPath(nonExistent);
    EXPECT_FALSE(canonicalNonExistent.empty());
}

// 测试路径比较
TEST_F(PathNormalizerTest, IsSamePath) {
    std::string file1 = (testDir / "test.txt").string();
    std::string file2 = (testDir / "subdir" / ".." / "test.txt").string();
    std::string file3 = (testDir / "subdir" / "nested.txt").string();
    
    // 相同文件的不同表示应该被识别为相同
    EXPECT_TRUE(PathNormalizer::isSamePath(file1, file2));
    
    // 不同文件应该被识别为不同
    EXPECT_FALSE(PathNormalizer::isSamePath(file1, file3));
    
    // 空路径测试
    EXPECT_TRUE(PathNormalizer::isSamePath("", ""));
    EXPECT_FALSE(PathNormalizer::isSamePath(file1, ""));
}

// 测试相对路径计算
TEST_F(PathNormalizerTest, GetRelativePath) {
    std::string from = testDir.string();
    std::string to = (testDir / "subdir" / "nested.txt").string();
    
    std::string relativePath = PathNormalizer::getRelativePath(from, to);
    
    // 相对路径应该是 subdir/nested.txt 或类似形式
    EXPECT_TRUE(relativePath.find("subdir") != std::string::npos);
    EXPECT_TRUE(relativePath.find("nested.txt") != std::string::npos);
    
    // 测试空路径
    EXPECT_EQ(PathNormalizer::getRelativePath("", to), PathNormalizer::normalize(to));
    EXPECT_EQ(PathNormalizer::getRelativePath(from, ""), "");
}

// 测试绝对路径检查
TEST_F(PathNormalizerTest, IsAbsolutePath) {
    // Unix风格绝对路径
    EXPECT_TRUE(PathNormalizer::isAbsolutePath("/home/user/file.txt"));
    EXPECT_TRUE(PathNormalizer::isAbsolutePath("/"));
    
    // Windows风格绝对路径
    EXPECT_TRUE(PathNormalizer::isAbsolutePath("C:\\Users\\file.txt"));
    EXPECT_TRUE(PathNormalizer::isAbsolutePath("D:\\"));
    
    // 相对路径
    EXPECT_FALSE(PathNormalizer::isAbsolutePath("file.txt"));
    EXPECT_FALSE(PathNormalizer::isAbsolutePath("./file.txt"));
    EXPECT_FALSE(PathNormalizer::isAbsolutePath("../file.txt"));
    EXPECT_FALSE(PathNormalizer::isAbsolutePath("subdir/file.txt"));
    
    // 空路径
    EXPECT_FALSE(PathNormalizer::isAbsolutePath(""));
}

// 测试文件名提取
TEST_F(PathNormalizerTest, GetFileName) {
    EXPECT_EQ(PathNormalizer::getFileName("/home/user/document.txt"), "document.txt");
    EXPECT_EQ(PathNormalizer::getFileName("C:\\Users\\file.doc"), "file.doc");
    EXPECT_EQ(PathNormalizer::getFileName("simple.txt"), "simple.txt");
    EXPECT_EQ(PathNormalizer::getFileName("/home/user/"), "");
    EXPECT_EQ(PathNormalizer::getFileName(""), "");
    
    // 测试只有扩展名的文件
    EXPECT_EQ(PathNormalizer::getFileName("/path/.hidden"), ".hidden");
    EXPECT_EQ(PathNormalizer::getFileName("/path/."), ".");
}

// 测试目录路径提取
TEST_F(PathNormalizerTest, GetDirectoryPath) {
    EXPECT_EQ(PathNormalizer::getDirectoryPath("/home/user/document.txt"), "/home/user");
    EXPECT_EQ(PathNormalizer::getDirectoryPath("C:\\Users\\file.doc"), "C:\\Users");
    EXPECT_EQ(PathNormalizer::getDirectoryPath("simple.txt"), "");
    EXPECT_EQ(PathNormalizer::getDirectoryPath("/home/user/"), "/home/user");
    EXPECT_EQ(PathNormalizer::getDirectoryPath(""), "");
    
    // 测试根目录
    EXPECT_EQ(PathNormalizer::getDirectoryPath("/file.txt"), "/");
}

// 测试文件存在性检查
TEST_F(PathNormalizerTest, Exists) {
    std::string existingFile = (testDir / "test.txt").string();
    std::string nonExistentFile = (testDir / "nonexistent.txt").string();
    
    EXPECT_TRUE(PathNormalizer::exists(existingFile));
    EXPECT_TRUE(PathNormalizer::exists(testDir.string())); // 目录也应该返回true
    EXPECT_FALSE(PathNormalizer::exists(nonExistentFile));
    EXPECT_FALSE(PathNormalizer::exists(""));
}

// 测试复杂路径场景
TEST_F(PathNormalizerTest, ComplexPathScenarios) {
    // 测试多级相对路径
    std::string complexPath = "../.././subdir/../subdir/./nested.txt";
    std::string normalized = PathNormalizer::normalize(complexPath);
    EXPECT_EQ(normalized, "../../subdir/nested.txt");
    
    // 测试混合分隔符（在支持的平台上）
    std::string mixedPath = "dir1/subdir\\..\\file.txt";
    std::string normalizedMixed = PathNormalizer::normalize(mixedPath);
    // 应该统一分隔符并移除 ..
    EXPECT_TRUE(normalizedMixed.find("..") == std::string::npos);
}

// 测试边界情况
TEST_F(PathNormalizerTest, EdgeCases) {
    // 测试只包含分隔符的路径
    EXPECT_EQ(PathNormalizer::normalize("/"), "/");
    EXPECT_EQ(PathNormalizer::normalize("\\"), "\\");
    
    // 测试只包含 . 和 .. 的路径
    EXPECT_EQ(PathNormalizer::normalize("."), ".");
    EXPECT_EQ(PathNormalizer::normalize(".."), "..");
    EXPECT_EQ(PathNormalizer::normalize("./.."), "..");
    EXPECT_EQ(PathNormalizer::normalize("../.."), "../..");
    
    // 测试连续分隔符
    std::string pathWithMultipleSlashes = "/home//user///file.txt";
    std::string normalizedSlashes = PathNormalizer::normalize(pathWithMultipleSlashes);
    // 应该移除多余的分隔符
    EXPECT_TRUE(normalizedSlashes.find("//") == std::string::npos);
}

// 测试性能（简单测试）
TEST_F(PathNormalizerTest, PerformanceTest) {
    std::string testPath = "/very/long/path/with/many/components/and/some/../redundant/./parts/file.txt";
    
    // 执行多次规范化操作，确保不会有明显的性能问题
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        PathNormalizer::normalize(testPath);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 1000次操作应该在合理时间内完成（比如1秒）
    EXPECT_LT(duration.count(), 1000);
} 