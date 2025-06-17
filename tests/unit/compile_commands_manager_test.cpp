/**
 * @file compile_commands_manager_test.cpp
 * @brief 编译命令管理器测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/config/compile_commands_manager.h>
#include <dlogcover/utils/file_utils.h>
#include <dlogcover/utils/log_utils.h>

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace dlogcover::config;
using namespace dlogcover::utils;

namespace dlogcover {
namespace test {

class CompileCommandsManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        testDir_ = "/tmp/dlogcover_compile_commands_test";
        if (std::filesystem::exists(testDir_)) {
            std::filesystem::remove_all(testDir_);
        }
        std::filesystem::create_directories(testDir_);
        
        buildDir_ = testDir_ + "/build";
        std::filesystem::create_directories(buildDir_);
        
        manager_ = std::make_unique<CompileCommandsManager>();
    }

    void TearDown() override {
        if (std::filesystem::exists(testDir_)) {
            std::filesystem::remove_all(testDir_);
        }
    }

    void createCMakeListsFile(const std::string& content = "") {
        std::string defaultContent = R"(
cmake_minimum_required(VERSION 3.10)
project(TestProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(test_app main.cpp)
)";
        
        std::ofstream file(testDir_ + "/CMakeLists.txt");
        file << (content.empty() ? defaultContent : content);
    }

    void createTestSourceFile(const std::string& filename, const std::string& content = "") {
        std::string defaultContent = R"(
#include <iostream>

int main() {
    std::cout << "Hello World" << std::endl;
    return 0;
}
)";
        
        std::ofstream file(testDir_ + "/" + filename);
        file << (content.empty() ? defaultContent : content);
    }

    void createCompileCommandsJson(const std::string& content) {
        std::ofstream file(buildDir_ + "/compile_commands.json");
        file << content;
    }

    std::string testDir_;
    std::string buildDir_;
    std::unique_ptr<CompileCommandsManager> manager_;
};

// 测试 CMake 项目检测
TEST_F(CompileCommandsManagerTest, DetectCMakeProject) {
    // 测试存在 CMakeLists.txt 的情况
    createCMakeListsFile();
    createTestSourceFile("main.cpp");
    
    // 注意：实际的 CMake 生成可能需要系统环境支持，这里主要测试逻辑
    // 在 CI 环境中可能需要 mock
    bool result = manager_->generateCompileCommands(testDir_, buildDir_, {});
    
    // 如果 CMake 不可用，应该返回 false 并设置错误
    if (!result) {
        EXPECT_FALSE(manager_->getError().empty());
        // 检查错误消息是否包含预期内容
        std::string error = manager_->getError();
        EXPECT_TRUE(error.find("CMake") != std::string::npos || 
                   error.find("cmake") != std::string::npos);
    }
}

TEST_F(CompileCommandsManagerTest, DetectMissingCMakeListsFile) {
    // 测试不存在 CMakeLists.txt 的情况
    bool result = manager_->generateCompileCommands(testDir_, buildDir_, {});
    
    EXPECT_FALSE(result);
    EXPECT_FALSE(manager_->getError().empty());
    EXPECT_TRUE(manager_->getError().find("CMakeLists.txt") != std::string::npos);
}

TEST_F(CompileCommandsManagerTest, DetectInvalidProjectDirectory) {
    // 测试无效路径
    std::string invalidDir = testDir_ + "/nonexistent_project_dir";
    bool result = manager_->generateCompileCommands(invalidDir, buildDir_, {});
    
    EXPECT_FALSE(result);
    EXPECT_FALSE(manager_->getError().empty());
}

// 测试 compile_commands.json 解析
TEST_F(CompileCommandsManagerTest, ParseValidCompileCommands) {
    std::string validJson = R"([
        {
            "directory": "/test/project",
            "command": "g++ -std=c++17 -I/usr/include -DTEST_MACRO -o main.o -c main.cpp",
            "file": "/test/project/main.cpp"
        },
        {
            "directory": "/test/project",
            "command": "g++ -std=c++17 -I/usr/include -I/usr/local/include -DDEBUG -o utils.o -c utils.cpp",
            "file": "/test/project/utils.cpp"
        }
    ])";
    
    createCompileCommandsJson(validJson);
    
    bool result = manager_->parseCompileCommands(buildDir_ + "/compile_commands.json");
    EXPECT_TRUE(result);
    EXPECT_TRUE(manager_->getError().empty());
    
    // 验证解析结果
    auto files = manager_->getAllFiles();
    EXPECT_EQ(files.size(), 2);
    
    // 测试获取编译信息
    CompileInfo info = manager_->getCompileInfoForFile("/test/project/main.cpp");
    EXPECT_EQ(info.file, "/test/project/main.cpp");
    EXPECT_EQ(info.directory, "/test/project");
    EXPECT_FALSE(info.command.empty());
    
    // 验证解析的包含路径和宏定义
    EXPECT_FALSE(info.include_paths.empty());
    EXPECT_FALSE(info.defines.empty());
    EXPECT_TRUE(std::find(info.include_paths.begin(), info.include_paths.end(), "/usr/include") != info.include_paths.end());
    EXPECT_TRUE(std::find(info.defines.begin(), info.defines.end(), "TEST_MACRO") != info.defines.end());
}

TEST_F(CompileCommandsManagerTest, ParseInvalidJsonFormat) {
    std::string invalidJson = R"({
        "not_an_array": "invalid"
    })";
    
    createCompileCommandsJson(invalidJson);
    
    bool result = manager_->parseCompileCommands(buildDir_ + "/compile_commands.json");
    EXPECT_FALSE(result);
    EXPECT_FALSE(manager_->getError().empty());
    EXPECT_TRUE(manager_->getError().find("数组") != std::string::npos);
}

TEST_F(CompileCommandsManagerTest, ParseMissingRequiredFields) {
    std::string incompleteJson = R"([
        {
            "directory": "/test/project",
            "command": "g++ -o main.o -c main.cpp"
            // 缺少 "file" 字段
        }
    ])";
    
    createCompileCommandsJson(incompleteJson);
    
    bool result = manager_->parseCompileCommands(buildDir_ + "/compile_commands.json");
    // 应该返回 false，因为没有有效的条目被解析
    EXPECT_FALSE(result);
}

TEST_F(CompileCommandsManagerTest, ParseNonexistentFile) {
    bool result = manager_->parseCompileCommands(testDir_ + "/nonexistent_file.json");
    EXPECT_FALSE(result);
    EXPECT_FALSE(manager_->getError().empty());
    EXPECT_TRUE(manager_->getError().find("不存在") != std::string::npos);
}

// 测试编译参数回退机制
TEST_F(CompileCommandsManagerTest, GetCompilerArgsExactMatch) {
    std::string validJson = R"([
        {
            "directory": "/test/project",
            "command": "g++ -std=c++17 -I/usr/include -DTEST_MACRO -o main.o -c main.cpp",
            "file": "/test/project/main.cpp"
        }
    ])";
    
    createCompileCommandsJson(validJson);
    manager_->parseCompileCommands(buildDir_ + "/compile_commands.json");
    
    auto args = manager_->getCompilerArgs("/test/project/main.cpp");
    EXPECT_FALSE(args.empty());
    
    // 验证包含了预期的编译参数
    bool hasStdFlag = false, hasIncludeFlag = false, hasDefineFlag = false;
    for (const auto& arg : args) {
        if (arg == "-std=c++17") hasStdFlag = true;
        if (arg == "-I/usr/include") hasIncludeFlag = true;
        if (arg == "-DTEST_MACRO") hasDefineFlag = true;
    }
    
    EXPECT_TRUE(hasStdFlag);
    EXPECT_TRUE(hasIncludeFlag);
    EXPECT_TRUE(hasDefineFlag);
}

TEST_F(CompileCommandsManagerTest, GetCompilerArgsSameNameMatch) {
    std::string validJson = R"([
        {
            "directory": "/different/project",
            "command": "g++ -std=c++17 -I/usr/include -DTEST_MACRO -o main.o -c main.cpp",
            "file": "/different/project/main.cpp"
        }
    ])";
    
    createCompileCommandsJson(validJson);
    manager_->parseCompileCommands(buildDir_ + "/compile_commands.json");
    
    // 请求不同路径但同名的文件
    auto args = manager_->getCompilerArgs("/another/path/main.cpp");
    EXPECT_FALSE(args.empty());
    
    // 应该找到同名文件的编译参数
    bool hasStdFlag = false;
    for (const auto& arg : args) {
        if (arg == "-std=c++17") hasStdFlag = true;
    }
    EXPECT_TRUE(hasStdFlag);
}

TEST_F(CompileCommandsManagerTest, GetCompilerArgsFallback) {
    // 不解析任何 compile_commands.json，测试回退机制
    auto args = manager_->getCompilerArgs("/some/unknown/file.cpp");
    EXPECT_FALSE(args.empty());
    
    // 验证包含基础的回退参数
    bool hasStdFlag = false, hasPicFlag = false, hasDebugFlag = false;
    for (const auto& arg : args) {
        if (arg == "-std=c++14") hasStdFlag = true;
        if (arg == "-fPIC") hasPicFlag = true;
        if (arg == "-g") hasDebugFlag = true;
    }
    
    EXPECT_TRUE(hasStdFlag);
    EXPECT_TRUE(hasPicFlag);
    EXPECT_TRUE(hasDebugFlag);
}

// 测试项目目录检测
TEST_F(CompileCommandsManagerTest, GetProjectDirectory) {
    // 创建嵌套的项目结构
    std::string subDir = testDir_ + "/src/subdir";
    std::filesystem::create_directories(subDir);
    createCMakeListsFile();
    
    std::string projectDir = manager_->getProjectDirectory(subDir + "/test.cpp");
    EXPECT_EQ(projectDir, testDir_);
}

TEST_F(CompileCommandsManagerTest, GetProjectDirectoryNotFound) {
    std::string projectDir = manager_->getProjectDirectory("/tmp/no_cmake_project/test.cpp");
    EXPECT_TRUE(projectDir.empty());
}

// 测试编译命令验证
TEST_F(CompileCommandsManagerTest, IsCompileCommandsValid) {
    // 测试有效的文件
    std::string validJson = R"([{"file": "test.cpp", "command": "g++", "directory": "/test"}])";
    createCompileCommandsJson(validJson);
    
    EXPECT_TRUE(manager_->isCompileCommandsValid(buildDir_ + "/compile_commands.json"));
    
    // 测试无效的文件
    std::string invalidJson = R"({"not": "array"})";
    createCompileCommandsJson(invalidJson);
    
    EXPECT_FALSE(manager_->isCompileCommandsValid(buildDir_ + "/compile_commands.json"));
    
    // 测试不存在的文件
    EXPECT_FALSE(manager_->isCompileCommandsValid(testDir_ + "/nonexistent_file.json"));
}

// 测试清理功能
TEST_F(CompileCommandsManagerTest, ClearFunction) {
    std::string validJson = R"([
        {
            "directory": "/test/project",
            "command": "g++ -o main.o -c main.cpp",
            "file": "/test/project/main.cpp"
        }
    ])";
    
    createCompileCommandsJson(validJson);
    manager_->parseCompileCommands(buildDir_ + "/compile_commands.json");
    
    EXPECT_FALSE(manager_->getAllFiles().empty());
    
    manager_->clear();
    
    EXPECT_TRUE(manager_->getAllFiles().empty());
    EXPECT_TRUE(manager_->getError().empty());
}

// 测试编译命令解析的边界情况
TEST_F(CompileCommandsManagerTest, ParseCompilerCommandWithQuotes) {
    std::string validJson = R"([
        {
            "directory": "/test/project",
            "command": "g++ -std=c++17 -I\"/path with spaces\" -DSTRING_MACRO=\"hello world\" -o main.o -c main.cpp",
            "file": "/test/project/main.cpp"
        }
    ])";
    
    createCompileCommandsJson(validJson);
    manager_->parseCompileCommands(buildDir_ + "/compile_commands.json");
    
    auto args = manager_->getCompilerArgs("/test/project/main.cpp");
    EXPECT_FALSE(args.empty());
    
    // 验证带引号的参数被正确解析
    bool hasQuotedInclude = false, hasQuotedDefine = false;
    for (const auto& arg : args) {
        if (arg == "-I/path with spaces") hasQuotedInclude = true;
        if (arg == "-DSTRING_MACRO=hello world") hasQuotedDefine = true;
    }
    
    EXPECT_TRUE(hasQuotedInclude);
    EXPECT_TRUE(hasQuotedDefine);
}

}  // namespace test
}  // namespace dlogcover 