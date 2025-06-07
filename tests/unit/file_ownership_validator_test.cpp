/**
 * @file file_ownership_validator_test.cpp
 * @brief 文件归属验证器单元测试
 * @author DLogCover Team
 * @date 2024
 */

#include <gtest/gtest.h>
#include "dlogcover/core/ast_analyzer/file_ownership_validator.h"
#include <filesystem>
#include <fstream>

using namespace dlogcover::core::ast_analyzer;

class FileOwnershipValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        validator = std::make_unique<FileOwnershipValidator>();
        
        // 创建测试目录结构
        testDir = std::filesystem::temp_directory_path() / "dlogcover_test";
        std::filesystem::create_directories(testDir);
        std::filesystem::create_directories(testDir / "src");
        std::filesystem::create_directories(testDir / "include");
        std::filesystem::create_directories(testDir / "tests");
        
        // 创建测试文件
        createTestFile(testDir / "src" / "main.cpp", "// main.cpp content");
        createTestFile(testDir / "src" / "utils.cpp", "// utils.cpp content");
        createTestFile(testDir / "include" / "utils.h", "// utils.h content");
        createTestFile(testDir / "tests" / "utils.cpp", "// test utils.cpp content");
        
        validator->setProjectRoot(testDir.string());
    }
    
    void TearDown() override {
        // 清理测试文件
        std::filesystem::remove_all(testDir);
    }
    
    void createTestFile(const std::filesystem::path& path, const std::string& content) {
        std::ofstream file(path);
        file << content;
        file.close();
    }
    
    std::unique_ptr<FileOwnershipValidator> validator;
    std::filesystem::path testDir;
};

// 测试严格模式验证
TEST_F(FileOwnershipValidatorTest, StrictValidation) {
    std::string file1 = (testDir / "src" / "main.cpp").string();
    std::string file2 = (testDir / "src" / "main.cpp").string();
    std::string file3 = (testDir / "src" / "utils.cpp").string();
    
    // 相同路径应该匹配
    auto result1 = validator->validateOwnership(file1, file2, 
        FileOwnershipValidator::ValidationLevel::STRICT);
    EXPECT_TRUE(result1.isOwned);
    EXPECT_EQ(result1.confidence, 1.0);
    
    // 不同路径应该不匹配
    auto result2 = validator->validateOwnership(file1, file3, 
        FileOwnershipValidator::ValidationLevel::STRICT);
    EXPECT_FALSE(result2.isOwned);
    EXPECT_EQ(result2.confidence, 0.0);
}

// 测试规范化模式验证
TEST_F(FileOwnershipValidatorTest, CanonicalValidation) {
    std::string file1 = (testDir / "src" / "main.cpp").string();
    std::string file2 = (testDir / "src" / ".." / "src" / "main.cpp").string();
    std::string file3 = (testDir / "src" / "utils.cpp").string();
    
    // 规范化后相同的路径应该匹配
    auto result1 = validator->validateOwnership(file1, file2, 
        FileOwnershipValidator::ValidationLevel::CANONICAL);
    EXPECT_TRUE(result1.isOwned);
    EXPECT_GT(result1.confidence, 0.9);
    
    // 不同文件应该不匹配
    auto result2 = validator->validateOwnership(file1, file3, 
        FileOwnershipValidator::ValidationLevel::CANONICAL);
    EXPECT_FALSE(result2.isOwned);
}

// 测试智能模式验证
TEST_F(FileOwnershipValidatorTest, SmartValidation) {
    std::string sourceFile = (testDir / "src" / "utils.cpp").string();
    std::string headerFile = (testDir / "include" / "utils.h").string();
    std::string testFile = (testDir / "tests" / "utils.cpp").string();
    
    // 头文件和源文件应该匹配（对应关系）
    auto result1 = validator->validateOwnership(sourceFile, headerFile, 
        FileOwnershipValidator::ValidationLevel::SMART);
    EXPECT_TRUE(result1.isOwned);
    EXPECT_GT(result1.confidence, 0.7);
    
    // 测试文件应该被排除
    validator->addExcludePattern(".*/tests/.*");
    auto result2 = validator->validateOwnership(sourceFile, testFile, 
        FileOwnershipValidator::ValidationLevel::SMART);
    EXPECT_FALSE(result2.isOwned);
}

// 测试模糊模式验证
TEST_F(FileOwnershipValidatorTest, FuzzyValidation) {
    std::string file1 = (testDir / "src" / "utils.cpp").string();
    std::string file2 = (testDir / "tests" / "utils.cpp").string();
    std::string file3 = (testDir / "src" / "main.cpp").string();
    
    // 相同文件名应该匹配（模糊模式）
    auto result1 = validator->validateOwnership(file1, file2, 
        FileOwnershipValidator::ValidationLevel::FUZZY);
    EXPECT_TRUE(result1.isOwned);
    EXPECT_GT(result1.confidence, 0.0);
    EXPECT_LT(result1.confidence, 0.5); // 模糊模式置信度较低
    
    // 不同文件名应该不匹配
    auto result2 = validator->validateOwnership(file1, file3, 
        FileOwnershipValidator::ValidationLevel::FUZZY);
    EXPECT_FALSE(result2.isOwned);
}

// 测试批量验证
TEST_F(FileOwnershipValidatorTest, BatchValidation) {
    std::string targetFile = (testDir / "src" / "main.cpp").string();
    std::vector<std::string> declFiles = {
        (testDir / "src" / "main.cpp").string(),
        (testDir / "src" / "utils.cpp").string(),
        (testDir / "include" / "utils.h").string()
    };
    
    auto results = validator->validateOwnershipBatch(targetFile, declFiles, 
        FileOwnershipValidator::ValidationLevel::SMART);
    
    EXPECT_EQ(results.size(), 3);
    EXPECT_TRUE(results[0].isOwned);  // 相同文件
    EXPECT_FALSE(results[1].isOwned); // 不同源文件
    // 第三个结果取决于智能匹配逻辑
}

// 测试缓存机制
TEST_F(FileOwnershipValidatorTest, CacheTest) {
    std::string file1 = (testDir / "src" / "main.cpp").string();
    std::string file2 = (testDir / "src" / "utils.cpp").string();
    
    validator->setCacheEnabled(true);
    
    // 第一次验证
    auto result1 = validator->validateOwnership(file1, file2, 
        FileOwnershipValidator::ValidationLevel::SMART);
    
    // 第二次验证（应该使用缓存）
    auto result2 = validator->validateOwnership(file1, file2, 
        FileOwnershipValidator::ValidationLevel::SMART);
    
    EXPECT_EQ(result1.isOwned, result2.isOwned);
    EXPECT_EQ(result1.confidence, result2.confidence);
    
    // 检查统计信息
    std::string stats = validator->getStatistics();
    EXPECT_TRUE(stats.find("Cache Hits: 1") != std::string::npos);
}

// 测试排除模式
TEST_F(FileOwnershipValidatorTest, ExcludePatterns) {
    std::string sourceFile = (testDir / "src" / "main.cpp").string();
    std::string testFile = (testDir / "tests" / "main.cpp").string();
    
    // 添加排除模式
    validator->addExcludePattern(".*/tests/.*");
    
    auto result = validator->validateOwnership(sourceFile, testFile, 
        FileOwnershipValidator::ValidationLevel::SMART);
    
    EXPECT_FALSE(result.isOwned);
    EXPECT_TRUE(result.reason.find("exclude pattern") != std::string::npos);
}

// 测试头文件和源文件对应关系
TEST_F(FileOwnershipValidatorTest, HeaderSourceCorrespondence) {
    // 创建更多测试文件
    createTestFile(testDir / "src" / "parser.cpp", "// parser.cpp");
    createTestFile(testDir / "include" / "parser.h", "// parser.h");
    createTestFile(testDir / "src" / "lexer.cxx", "// lexer.cxx");
    createTestFile(testDir / "include" / "lexer.hpp", "// lexer.hpp");
    
    // 测试 .cpp 和 .h 对应关系
    auto result1 = validator->validateOwnership(
        (testDir / "src" / "parser.cpp").string(),
        (testDir / "include" / "parser.h").string(),
        FileOwnershipValidator::ValidationLevel::SMART);
    EXPECT_TRUE(result1.isOwned);
    
    // 测试 .cxx 和 .hpp 对应关系
    auto result2 = validator->validateOwnership(
        (testDir / "src" / "lexer.cxx").string(),
        (testDir / "include" / "lexer.hpp").string(),
        FileOwnershipValidator::ValidationLevel::SMART);
    EXPECT_TRUE(result2.isOwned);
}

// 测试不存在文件的处理
TEST_F(FileOwnershipValidatorTest, NonExistentFiles) {
    std::string existingFile = (testDir / "src" / "main.cpp").string();
    std::string nonExistentFile = (testDir / "src" / "nonexistent.cpp").string();
    
    auto result = validator->validateOwnership(existingFile, nonExistentFile, 
        FileOwnershipValidator::ValidationLevel::CANONICAL);
    
    // 应该能够处理不存在的文件而不崩溃
    EXPECT_FALSE(result.isOwned);
}

// 测试统计信息
TEST_F(FileOwnershipValidatorTest, Statistics) {
    std::string file1 = (testDir / "src" / "main.cpp").string();
    std::string file2 = (testDir / "src" / "utils.cpp").string();
    
    // 执行几次验证
    validator->validateOwnership(file1, file1, FileOwnershipValidator::ValidationLevel::STRICT);
    validator->validateOwnership(file1, file2, FileOwnershipValidator::ValidationLevel::CANONICAL);
    validator->validateOwnership(file1, file2, FileOwnershipValidator::ValidationLevel::SMART);
    
    std::string stats = validator->getStatistics();
    
    EXPECT_TRUE(stats.find("Total Validations: 3") != std::string::npos);
    EXPECT_TRUE(stats.find("Strict Matches:") != std::string::npos);
    EXPECT_TRUE(stats.find("Canonical Matches:") != std::string::npos);
    EXPECT_TRUE(stats.find("Smart Matches:") != std::string::npos);
}

// 测试验证级别字符串转换
TEST_F(FileOwnershipValidatorTest, ValidationLevelConversion) {
    EXPECT_EQ(validationLevelToString(FileOwnershipValidator::ValidationLevel::STRICT), "STRICT");
    EXPECT_EQ(validationLevelToString(FileOwnershipValidator::ValidationLevel::CANONICAL), "CANONICAL");
    EXPECT_EQ(validationLevelToString(FileOwnershipValidator::ValidationLevel::SMART), "SMART");
    EXPECT_EQ(validationLevelToString(FileOwnershipValidator::ValidationLevel::FUZZY), "FUZZY");
    
    EXPECT_EQ(stringToValidationLevel("STRICT"), FileOwnershipValidator::ValidationLevel::STRICT);
    EXPECT_EQ(stringToValidationLevel("CANONICAL"), FileOwnershipValidator::ValidationLevel::CANONICAL);
    EXPECT_EQ(stringToValidationLevel("SMART"), FileOwnershipValidator::ValidationLevel::SMART);
    EXPECT_EQ(stringToValidationLevel("FUZZY"), FileOwnershipValidator::ValidationLevel::FUZZY);
    EXPECT_EQ(stringToValidationLevel("UNKNOWN"), FileOwnershipValidator::ValidationLevel::SMART); // 默认值
} 