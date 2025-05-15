/**
 * @file file_utils_test.cpp
 * @brief 文件工具函数的单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/utils/file_utils.h>
#include <filesystem>
#include <fstream>

namespace dlogcover {
namespace utils {
namespace test {

// 测试文件路径拼接函数
TEST(FileUtilsTest, JoinPaths) {
    // Arrange
    std::string path1 = "/path/to";
    std::string path2 = "file.txt";
    
    // Act
    std::string result = FileUtils::joinPaths(path1, path2);
    
    // Assert
    EXPECT_EQ("/path/to/file.txt", result);
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

// 测试创建临时文件
TEST(FileUtilsTest, CreateTempFile) {
    // Arrange
    std::string content = "Test content";
    
    // Act
    std::string tempFile = FileUtils::createTempFile(content);
    
    // Assert
    EXPECT_FALSE(tempFile.empty());
    
    // 验证文件内容
    std::ifstream file(tempFile);
    ASSERT_TRUE(file.is_open());
    
    std::string fileContent((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    EXPECT_EQ(content, fileContent);
    
    // 清理
    file.close();
    std::filesystem::remove(tempFile);
}

// 添加更多需要测试的函数...

} // namespace test
} // namespace utils
} // namespace dlogcover 