/**
 * @file result_test.cpp
 * @brief Result模板类单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/common/result.h>
#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <memory>
#include <string>
#include <chrono>

using namespace dlogcover;
namespace ast = dlogcover::core::ast_analyzer;

class ResultTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// 测试基本bool类型的Result
TEST_F(ResultTest, BasicBoolResult) {
    // 测试成功情况
    bool success = true;
    auto result1 = makeSuccess<bool, ast::ASTAnalyzerError>(success);
    EXPECT_TRUE(result1.isSuccess());
    EXPECT_FALSE(result1.hasError());
    EXPECT_TRUE(result1.value());
    
    // 测试move语义
    auto result2 = makeSuccess<bool, ast::ASTAnalyzerError>(std::move(success));
    EXPECT_TRUE(result2.isSuccess());
    EXPECT_TRUE(result2.value());
    
    // 测试临时值
    auto result3 = makeSuccess<bool, ast::ASTAnalyzerError>(true);
    EXPECT_TRUE(result3.isSuccess());
    EXPECT_TRUE(result3.value());
    
    // 测试错误情况
    auto errorResult = makeError<bool, ast::ASTAnalyzerError>(
        ast::ASTAnalyzerError::COMPILATION_ERROR, "Test error");
    EXPECT_FALSE(errorResult.isSuccess());
    EXPECT_TRUE(errorResult.hasError());
    EXPECT_EQ(errorResult.error(), ast::ASTAnalyzerError::COMPILATION_ERROR);
    EXPECT_EQ(errorResult.errorMessage(), "Test error");
}

// 测试字符串类型的Result
TEST_F(ResultTest, StringResult) {
    std::string testStr = "Hello World";
    auto result1 = makeSuccess<std::string, ast::ASTAnalyzerError>(testStr);
    EXPECT_TRUE(result1.isSuccess());
    EXPECT_EQ(result1.value(), "Hello World");
    
    // 测试move语义
    std::string moveStr = "Move Test";
    auto result2 = makeSuccess<std::string, ast::ASTAnalyzerError>(std::move(moveStr));
    EXPECT_TRUE(result2.isSuccess());
    EXPECT_EQ(result2.value(), "Move Test");
    
    // 测试临时字符串
    auto result3 = makeSuccess<std::string, ast::ASTAnalyzerError>(std::string("Temp String"));
    EXPECT_TRUE(result3.isSuccess());
    EXPECT_EQ(result3.value(), "Temp String");
}

// 测试智能指针类型的Result
TEST_F(ResultTest, SmartPointerResult) {
    auto ptr = std::make_unique<int>(42);
    auto result1 = makeSuccess<std::unique_ptr<int>, ast::ASTAnalyzerError>(std::move(ptr));
    EXPECT_TRUE(result1.isSuccess());
    EXPECT_NE(result1.value(), nullptr);
    EXPECT_EQ(*result1.value(), 42);
}

// 测试引用类型推导修复
TEST_F(ResultTest, ReferenceTypeDeduction) {
    // 这些调用在修复前会导致编译错误
    bool localBool = false;
    auto result1 = makeSuccess<bool, ast::ASTAnalyzerError>(localBool);  // 左值引用推导
    EXPECT_TRUE(result1.isSuccess());
    EXPECT_FALSE(result1.value());
    
    int localInt = 123;
    auto result2 = makeSuccess<int, ast::ASTAnalyzerError>(localInt);
    EXPECT_TRUE(result2.isSuccess());
    EXPECT_EQ(result2.value(), 123);
}

// 测试Result的拷贝和移动
TEST_F(ResultTest, CopyAndMove) {
    auto original = makeSuccess<std::string, ast::ASTAnalyzerError>(std::string("Original"));
    
    // 拷贝构造
    auto copied = original;
    EXPECT_TRUE(copied.isSuccess());
    EXPECT_EQ(copied.value(), "Original");
    EXPECT_TRUE(original.isSuccess());  // 原对象应该仍然有效
    
    // 移动构造
    auto moved = std::move(original);
    EXPECT_TRUE(moved.isSuccess());
    EXPECT_EQ(moved.value(), "Original");
}

// 测试错误处理
TEST_F(ResultTest, ErrorHandling) {
    auto errorResult = makeError<std::string, ast::ASTAnalyzerError>(
        ast::ASTAnalyzerError::FILE_NOT_FOUND, "File not found");
    
    EXPECT_FALSE(errorResult.isSuccess());
    EXPECT_TRUE(errorResult.hasError());
    EXPECT_TRUE(errorResult.isError());
    EXPECT_FALSE(static_cast<bool>(errorResult));
    
    EXPECT_EQ(errorResult.error(), ast::ASTAnalyzerError::FILE_NOT_FOUND);
    EXPECT_EQ(errorResult.errorMessage(), "File not found");
}

// 测试模板类型推导
TEST_F(ResultTest, TemplateTypeDeduction) {
    // 测试自动类型推导
    auto result1 = makeSuccess<bool, ast::ASTAnalyzerError>(true);
    static_assert(std::is_same_v<decltype(result1), Result<bool, ast::ASTAnalyzerError>>);
    
    auto result2 = makeSuccess<int, ast::ASTAnalyzerError>(42);
    static_assert(std::is_same_v<decltype(result2), Result<int, ast::ASTAnalyzerError>>);
    
    auto result3 = makeSuccess<std::string, ast::ASTAnalyzerError>(std::string("test"));
    static_assert(std::is_same_v<decltype(result3), Result<std::string, ast::ASTAnalyzerError>>);
}

// 性能测试：确保修复不影响性能
TEST_F(ResultTest, PerformanceTest) {
    const int iterations = 10000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        bool value = (i % 2 == 0);
        auto result = makeSuccess<bool, ast::ASTAnalyzerError>(value);
        EXPECT_TRUE(result.isSuccess());
        EXPECT_EQ(result.value(), value);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 性能要求：10000次操作应该在合理时间内完成（比如10ms）
    EXPECT_LT(duration.count(), 10000);  // 小于10ms
} 