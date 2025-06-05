/**
 * @file cmake_parser_test.cpp
 * @brief CMake解析器单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/utils/cmake_parser.h>
#include <dlogcover/utils/cmake_types.h>

#include <filesystem>
#include <fstream>
#include <sstream>

using namespace dlogcover::utils;

class CMakeParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser_ = std::make_unique<CMakeParser>();
        testDir_ = std::filesystem::temp_directory_path() / "cmake_parser_test";
        std::filesystem::create_directories(testDir_);
    }

    void TearDown() override {
        if (std::filesystem::exists(testDir_)) {
            std::filesystem::remove_all(testDir_);
        }
    }

    void createTestCMakeFile(const std::string& content) {
        testCMakeFile_ = testDir_ / "CMakeLists.txt";
        std::ofstream file(testCMakeFile_);
        file << content;
        file.close();
    }

    std::unique_ptr<CMakeParser> parser_;
    std::filesystem::path testDir_;
    std::filesystem::path testCMakeFile_;
};

TEST_F(CMakeParserTest, ParseBasicProject) {
    std::string content = R"(
cmake_minimum_required(VERSION 3.10)
project(TestProject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
)";

    createTestCMakeFile(content);
    
    auto result = parser_->parse(testCMakeFile_.string());
    ASSERT_FALSE(result.hasError()) << "解析失败: " << result.errorMessage();
    
    const auto& parseResult = result.value();
    EXPECT_EQ(parseResult.projectName, "TestProject");
    EXPECT_EQ(parseResult.projectVersion, "1.0.0");
    EXPECT_EQ(parseResult.cxxStandard, "17");
    EXPECT_TRUE(parseResult.isValid());
}

TEST_F(CMakeParserTest, ParseIncludeDirectories) {
    std::string content = R"(
cmake_minimum_required(VERSION 3.10)
project(TestProject)

include_directories(./include)
include_directories(/usr/local/include)
include_directories(${CMAKE_SOURCE_DIR}/external)
)";

    createTestCMakeFile(content);
    
    auto result = parser_->parse(testCMakeFile_.string());
    ASSERT_FALSE(result.hasError());
    
    const auto& parseResult = result.value();
    EXPECT_GE(parseResult.includeDirectories.size(), 3);
    
    // 检查是否包含预期的目录
    bool foundInclude = false, foundUsrLocal = false, foundExternal = false;
    for (const auto& dir : parseResult.includeDirectories) {
        if (dir.find("include") != std::string::npos) foundInclude = true;
        if (dir.find("/usr/local/include") != std::string::npos) foundUsrLocal = true;
        if (dir.find("external") != std::string::npos) foundExternal = true;
    }
    
    EXPECT_TRUE(foundInclude);
    EXPECT_TRUE(foundUsrLocal);
    EXPECT_TRUE(foundExternal);
}

TEST_F(CMakeParserTest, ParseCompileDefinitions) {
    std::string content = R"(
cmake_minimum_required(VERSION 3.10)
project(TestProject)

add_definitions(-DDEBUG)
add_definitions(-DVERSION="1.0")
add_definitions(-D_GNU_SOURCE)
)";

    createTestCMakeFile(content);
    
    auto result = parser_->parse(testCMakeFile_.string());
    ASSERT_FALSE(result.hasError());
    
    const auto& parseResult = result.value();
    EXPECT_GE(parseResult.compileDefinitions.size(), 3);
    
    // 检查是否包含预期的定义
    bool foundDebug = false, foundVersion = false, foundGnuSource = false;
    for (const auto& def : parseResult.compileDefinitions) {
        if (def.find("DEBUG") != std::string::npos) foundDebug = true;
        if (def.find("VERSION") != std::string::npos) foundVersion = true;
        if (def.find("_GNU_SOURCE") != std::string::npos) foundGnuSource = true;
    }
    
    EXPECT_TRUE(foundDebug);
    EXPECT_TRUE(foundVersion);
    EXPECT_TRUE(foundGnuSource);
}

TEST_F(CMakeParserTest, ParseExecutableTarget) {
    std::string content = R"(
cmake_minimum_required(VERSION 3.10)
project(TestProject)

add_executable(test_app
    src/main.cpp
    src/utils.cpp
    include/utils.h
)

target_include_directories(test_app PRIVATE ./include)
target_compile_definitions(test_app PRIVATE APP_VERSION="1.0")
target_compile_options(test_app PRIVATE -Wall -Wextra)
)";

    createTestCMakeFile(content);
    
    auto result = parser_->parse(testCMakeFile_.string());
    ASSERT_FALSE(result.hasError());
    
    const auto& parseResult = result.value();
    EXPECT_EQ(parseResult.targets.size(), 1);
    
    auto it = parseResult.targets.find("test_app");
    ASSERT_NE(it, parseResult.targets.end());
    
    const auto& target = it->second;
    EXPECT_EQ(target.name, "test_app");
    EXPECT_EQ(target.type, "EXECUTABLE");
    EXPECT_GE(target.sources.size(), 3);
    EXPECT_GE(target.includeDirectories.size(), 1);
    EXPECT_GE(target.compileDefinitions.size(), 1);
    EXPECT_GE(target.compileOptions.size(), 2);
}

TEST_F(CMakeParserTest, ParseLibraryTarget) {
    std::string content = R"(
cmake_minimum_required(VERSION 3.10)
project(TestProject)

add_library(test_lib STATIC
    src/lib.cpp
    include/lib.h
)

target_include_directories(test_lib PUBLIC ./include)
target_compile_definitions(test_lib PUBLIC LIB_EXPORT)
target_link_libraries(test_lib PRIVATE pthread)
)";

    createTestCMakeFile(content);
    
    auto result = parser_->parse(testCMakeFile_.string());
    ASSERT_FALSE(result.hasError());
    
    const auto& parseResult = result.value();
    EXPECT_EQ(parseResult.targets.size(), 1);
    
    auto it = parseResult.targets.find("test_lib");
    ASSERT_NE(it, parseResult.targets.end());
    
    const auto& target = it->second;
    EXPECT_EQ(target.name, "test_lib");
    EXPECT_EQ(target.type, "STATIC_LIBRARY");
    EXPECT_GE(target.sources.size(), 2);
    EXPECT_GE(target.includeDirectories.size(), 1);
    EXPECT_GE(target.compileDefinitions.size(), 1);
    EXPECT_GE(target.linkLibraries.size(), 1);
}

TEST_F(CMakeParserTest, ParseVariables) {
    std::string content = R"(
cmake_minimum_required(VERSION 3.10)
project(TestProject)

set(MY_VAR "test_value")
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_STANDARD 20)
)";

    createTestCMakeFile(content);
    
    auto result = parser_->parse(testCMakeFile_.string());
    ASSERT_FALSE(result.hasError());
    
    const auto& parseResult = result.value();
    
    // 检查变量设置
    EXPECT_EQ(parseResult.cxxStandard, "20");
    EXPECT_GE(parseResult.compileOptions.size(), 2);
    
    // 检查编译选项是否包含预期的标志
    bool foundWall = false, foundWextra = false;
    for (const auto& option : parseResult.compileOptions) {
        if (option == "-Wall") foundWall = true;
        if (option == "-Wextra") foundWextra = true;
    }
    
    EXPECT_TRUE(foundWall);
    EXPECT_TRUE(foundWextra);
}

TEST_F(CMakeParserTest, GetAllCompilerArgs) {
    std::string content = R"(
cmake_minimum_required(VERSION 3.10)
project(TestProject)

set(CMAKE_CXX_STANDARD 17)
include_directories(./include)
add_definitions(-DDEBUG)
set(CMAKE_CXX_FLAGS "-Wall")
)";

    createTestCMakeFile(content);
    
    auto result = parser_->parse(testCMakeFile_.string());
    ASSERT_FALSE(result.hasError());
    
    const auto& parseResult = result.value();
    auto args = parseResult.getAllCompilerArgs();
    
    EXPECT_GT(args.size(), 0);
    
    // 检查是否包含预期的参数
    bool foundStd = false, foundInclude = false, foundDefine = false, foundWall = false;
    for (const auto& arg : args) {
        if (arg == "-std=c++17") foundStd = true;
        if (arg.find("-I") == 0 && arg.find("include") != std::string::npos) foundInclude = true;
        if (arg.find("-D") == 0 && arg.find("DEBUG") != std::string::npos) foundDefine = true;
        if (arg == "-Wall") foundWall = true;
    }
    
    EXPECT_TRUE(foundStd);
    EXPECT_TRUE(foundInclude);
    EXPECT_TRUE(foundDefine);
    EXPECT_TRUE(foundWall);
}

TEST_F(CMakeParserTest, GetTargetCompilerArgs) {
    std::string content = R"(
cmake_minimum_required(VERSION 3.10)
project(TestProject)

set(CMAKE_CXX_STANDARD 17)
include_directories(./global_include)

add_executable(test_app src/main.cpp)
target_include_directories(test_app PRIVATE ./target_include)
target_compile_definitions(test_app PRIVATE TARGET_DEFINE)
target_compile_options(test_app PRIVATE -O2)
)";

    createTestCMakeFile(content);
    
    auto result = parser_->parse(testCMakeFile_.string());
    ASSERT_FALSE(result.hasError());
    
    const auto& parseResult = result.value();
    auto args = parseResult.getTargetCompilerArgs("test_app");
    
    EXPECT_GT(args.size(), 0);
    
    // 检查是否包含全局和目标特定的参数
    bool foundStd = false, foundGlobalInclude = false, foundTargetInclude = false;
    bool foundTargetDefine = false, foundO2 = false;
    
    for (const auto& arg : args) {
        if (arg == "-std=c++17") foundStd = true;
        if (arg.find("-I") == 0 && arg.find("global_include") != std::string::npos) foundGlobalInclude = true;
        if (arg.find("-I") == 0 && arg.find("target_include") != std::string::npos) foundTargetInclude = true;
        if (arg.find("-D") == 0 && arg.find("TARGET_DEFINE") != std::string::npos) foundTargetDefine = true;
        if (arg == "-O2") foundO2 = true;
    }
    
    EXPECT_TRUE(foundStd);
    EXPECT_TRUE(foundGlobalInclude);
    EXPECT_TRUE(foundTargetInclude);
    EXPECT_TRUE(foundTargetDefine);
    EXPECT_TRUE(foundO2);
}

TEST_F(CMakeParserTest, ParseContent) {
    std::string content = R"(
cmake_minimum_required(VERSION 3.10)
project(TestProject VERSION 2.0)
set(CMAKE_CXX_STANDARD 20)
)";

    auto result = parser_->parseContent(content, testDir_.string());
    ASSERT_FALSE(result.hasError());
    
    const auto& parseResult = result.value();
    EXPECT_EQ(parseResult.projectName, "TestProject");
    EXPECT_EQ(parseResult.projectVersion, "2.0");
    EXPECT_EQ(parseResult.cxxStandard, "20");
    EXPECT_EQ(parseResult.sourceDir, testDir_.string());
}

TEST_F(CMakeParserTest, HandleNonExistentFile) {
    std::string nonExistentFile = testDir_ / "non_existent.txt";
    
    auto result = parser_->parse(nonExistentFile);
    EXPECT_TRUE(result.hasError());
    EXPECT_EQ(result.error(), CMakeParserError::FILE_NOT_FOUND);
}

TEST_F(CMakeParserTest, HandleInvalidContent) {
    std::string content = R"(
invalid cmake syntax here
this is not valid cmake
)";

    auto result = parser_->parseContent(content, testDir_.string());
    // 解析器应该能够处理无效内容而不崩溃
    // 可能返回空的解析结果，但不应该出错
    EXPECT_FALSE(result.hasError());
}

TEST_F(CMakeParserTest, VerboseLogging) {
    parser_->setVerboseLogging(true);
    
    std::string content = R"(
cmake_minimum_required(VERSION 3.10)
project(TestProject)
)";

    auto result = parser_->parseContent(content, testDir_.string());
    EXPECT_FALSE(result.hasError());
    
    // 测试日志设置不会影响解析结果
    const auto& parseResult = result.value();
    EXPECT_EQ(parseResult.projectName, "TestProject");
}

TEST_F(CMakeParserTest, VariableExpansion) {
    parser_->setVariable("TEST_VAR", "test_value");
    
    std::string testVar = parser_->getVariable("TEST_VAR");
    EXPECT_EQ(testVar, "test_value");
    
    parser_->clearVariables();
    testVar = parser_->getVariable("TEST_VAR");
    EXPECT_TRUE(testVar.empty());
}

// 主函数
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 