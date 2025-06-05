/**
 * @file file_utils_test.cpp
 * @brief 文件工具函数的单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/utils/file_utils.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace dlogcover {
namespace utils {
namespace test {

// 测试文件路径拼接函数
TEST(FileUtilsTest, JoinPaths) {
    // Arrange & Act & Assert
    EXPECT_EQ("/path/to/file", FileUtils::joinPaths("/path/to", "file"));
    EXPECT_EQ("/path/to/file", FileUtils::joinPaths("/path/to/", "file"));
    EXPECT_EQ("/file", FileUtils::joinPaths("/path/to", "/file"));
    EXPECT_EQ("/file", FileUtils::joinPaths("/path/to/", "/file"));
}

// 测试文件扩展名检查函数
TEST(FileUtilsTest, HasExtension) {
    // Arrange
    std::string file1 = "test.cpp";
    std::string file2 = "test.h";
    std::string file3 = "test";

    // Act & Assert
    EXPECT_TRUE(FileUtils::hasExtension(file1, ".cpp"));
    EXPECT_FALSE(FileUtils::hasExtension(file1, ".h"));
    EXPECT_TRUE(FileUtils::hasExtension(file2, ".h"));
    EXPECT_FALSE(FileUtils::hasExtension(file3, ".cpp"));
}

// 测试文件扩展名获取函数
TEST(FileUtilsTest, GetFileExtension) {
    // Arrange & Act & Assert
    EXPECT_EQ(".cpp", FileUtils::getFileExtension("test.cpp"));
    EXPECT_EQ(".h", FileUtils::getFileExtension("test.h"));
    EXPECT_EQ("", FileUtils::getFileExtension("test"));
    EXPECT_EQ(".txt", FileUtils::getFileExtension("/path/to/file.txt"));
}

// 测试文件名获取函数
TEST(FileUtilsTest, GetFileName) {
    // Arrange & Act & Assert
    EXPECT_EQ("test", FileUtils::getFileName("test.cpp"));
    EXPECT_EQ("file", FileUtils::getFileName("/path/to/file.txt"));
    EXPECT_EQ("test", FileUtils::getFileName("test"));
}

// 测试目录名获取函数
TEST(FileUtilsTest, GetDirectoryName) {
    // Arrange & Act & Assert
    EXPECT_EQ("", FileUtils::getDirectoryName("test.cpp"));
    EXPECT_EQ("/path/to", FileUtils::getDirectoryName("/path/to/file.txt"));

    // 这里注意：根据当前目录的不同，结果可能有所不同
    std::string dirPath = FileUtils::getDirectoryName("dir/file.txt");
    EXPECT_TRUE(dirPath == "dir" || dirPath == "./dir");
}

// 测试创建临时文件
TEST(FileUtilsTest, CreateTempFile) {
    // Arrange
    std::string content = "Test content";

    // Act - 先创建空临时文件，然后写入内容
    std::string tempFile = FileUtils::createTempFile("dlogcover_test_", TempFileType::EMPTY);
    ASSERT_TRUE(FileUtils::writeFile(tempFile, content));

    // Assert
    EXPECT_FALSE(tempFile.empty());

    // 验证文件内容
    std::string readContent;
    ASSERT_TRUE(FileUtils::readFile(tempFile, readContent));
    EXPECT_EQ(content, readContent);

    // 清理
    std::filesystem::remove(tempFile);
}

// 测试创建带前缀的临时文件
TEST(FileUtilsTest, CreateTempFileWithPrefix) {
    // Arrange
    std::string prefix = "test_prefix";

    // Act - 测试空文件
    std::string emptyTempFile = FileUtils::createTempFile(prefix, TempFileType::EMPTY);

    // Assert
    EXPECT_FALSE(emptyTempFile.empty());
    EXPECT_TRUE(FileUtils::fileExists(emptyTempFile));
    EXPECT_TRUE(emptyTempFile.find(prefix) != std::string::npos);
    EXPECT_EQ(0, FileUtils::getFileSize(emptyTempFile));

    // Act - 测试带内容的文件
    std::string contentTempFile = FileUtils::createTempFile(prefix, TempFileType::WITH_CONTENT);

    // Assert
    EXPECT_FALSE(contentTempFile.empty());
    EXPECT_TRUE(FileUtils::fileExists(contentTempFile));
    EXPECT_TRUE(contentTempFile.find(prefix) != std::string::npos);
    EXPECT_GT(FileUtils::getFileSize(contentTempFile), 0);

    // 清理
    std::filesystem::remove(emptyTempFile);
    std::filesystem::remove(contentTempFile);
}

// 测试临时文件清理
TEST(FileUtilsTest, CleanupTempFiles) {
    // Arrange - 创建临时文件并写入内容
    std::string tempFile1 = FileUtils::createTempFile("test_prefix1_", TempFileType::EMPTY);
    std::string tempFile2 = FileUtils::createTempFile("test_prefix2_", TempFileType::EMPTY);
    
    ASSERT_TRUE(FileUtils::writeFile(tempFile1, "test_content_1"));
    ASSERT_TRUE(FileUtils::writeFile(tempFile2, "test_content_2"));

    // 确认文件存在
    EXPECT_TRUE(FileUtils::fileExists(tempFile1));
    EXPECT_TRUE(FileUtils::fileExists(tempFile2));

    // Act
    FileUtils::cleanupTempFiles();

    // Assert
    EXPECT_FALSE(FileUtils::fileExists(tempFile1));
    EXPECT_FALSE(FileUtils::fileExists(tempFile2));
}

// 测试文件路径规范化
TEST(FileUtilsTest, NormalizePath) {
    // Arrange & Act & Assert
    EXPECT_EQ("/path/to/file", FileUtils::normalizePath("/path/to/file"));
    EXPECT_EQ("/path/to/file", FileUtils::normalizePath("/path/to//file"));
    EXPECT_EQ("/path/to/file", FileUtils::normalizePath("/path/./to/file"));
    EXPECT_EQ("/path/file", FileUtils::normalizePath("/path/to/../file"));
}

// 测试获取相对路径
TEST(FileUtilsTest, GetRelativePath) {
    // Arrange & Act & Assert
    EXPECT_EQ("file.txt", FileUtils::getRelativePath("/path/to/file.txt", "/path/to"));
    EXPECT_EQ("to/file.txt", FileUtils::getRelativePath("/path/to/file.txt", "/path"));
    EXPECT_EQ("../sibling/file.txt", FileUtils::getRelativePath("/path/sibling/file.txt", "/path/to"));
}

// 测试创建目录（如果不存在）
TEST(FileUtilsTest, CreateDirectoryIfNotExists) {
    // Arrange
    std::string tempDir = std::filesystem::temp_directory_path().string() + "/dlogcover_test_dir";

    // 确保目录不存在
    if (FileUtils::directoryExists(tempDir)) {
        std::filesystem::remove_all(tempDir);
    }

    // Act & Assert - 目录不存在时创建
    EXPECT_FALSE(FileUtils::directoryExists(tempDir));
    EXPECT_TRUE(FileUtils::createDirectoryIfNotExists(tempDir));
    EXPECT_TRUE(FileUtils::directoryExists(tempDir));

    // Act & Assert - 目录已存在时返回true
    EXPECT_TRUE(FileUtils::createDirectoryIfNotExists(tempDir));

    // 清理
    std::filesystem::remove_all(tempDir);
}

// 测试文件大小获取
TEST(FileUtilsTest, GetFileSize) {
    // Arrange - 创建临时文件并写入指定内容
    std::string content = "Test content with specific size";
    std::string tempFile = FileUtils::createTempFile("size_test_", TempFileType::EMPTY);
    ASSERT_TRUE(FileUtils::writeFile(tempFile, content));

    // Act
    size_t size = FileUtils::getFileSize(tempFile);

    // Assert
    EXPECT_EQ(31, size); // "Test content with specific size" 的实际长度是31字节

    // 清理
    std::filesystem::remove(tempFile);
}

// 测试文件操作组合
TEST(FileUtilsTest, FileOperationsCombined) {
    // Arrange
    std::string tempDir = std::filesystem::temp_directory_path().string() + "/dlogcover_test_combined";
    FileUtils::createDirectoryIfNotExists(tempDir);

    std::string filePath = FileUtils::joinPaths(tempDir, "test.txt");
    std::string content = "Test content for combined operations";

    // Act - 写入文件
    EXPECT_TRUE(FileUtils::writeFile(filePath, content));

    // Assert - 文件存在且内容正确
    EXPECT_TRUE(FileUtils::fileExists(filePath));

    std::string readContent;
    EXPECT_TRUE(FileUtils::readFile(filePath, readContent));
    EXPECT_EQ(content, readContent);

    // Act & Assert - 文件属性
    EXPECT_EQ(".txt", FileUtils::getFileExtension(filePath));
    EXPECT_EQ("test", FileUtils::getFileName(filePath));
    EXPECT_EQ(tempDir, FileUtils::getDirectoryName(filePath));
    EXPECT_EQ(content.size(), FileUtils::getFileSize(filePath));

    // 清理
    std::filesystem::remove_all(tempDir);
}

} // namespace test
} // namespace utils
} // namespace dlogcover
