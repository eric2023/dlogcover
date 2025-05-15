cmake_minimum_required(VERSION 3.10)
project(dlogcover VERSION 0.1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

# 设置编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# 检查LLVM依赖
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM_LIBRARY_DIRS: ${LLVM_LIBRARY_DIRS}")
message(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")

# 添加LLVM定义和包含目录
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
link_directories(${LLVM_LIBRARY_DIRS})

# 查找Clang库
find_library(CLANG_TOOLING_LIB clangTooling PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_BASIC_LIB clangBasic PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_AST_MATCHERS_LIB clangASTMatchers PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_AST_LIB clangAST PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_FRONTEND_LIB clangFrontend PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_LEX_LIB clangLex PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_PARSE_LIB clangParse PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_SEMA_LIB clangSema PATHS ${LLVM_LIBRARY_DIRS})

# 显示找到的Clang库路径
message(STATUS "CLANG_TOOLING_LIB: ${CLANG_TOOLING_LIB}")
message(STATUS "CLANG_BASIC_LIB: ${CLANG_BASIC_LIB}")
message(STATUS "CLANG_AST_MATCHERS_LIB: ${CLANG_AST_MATCHERS_LIB}")
message(STATUS "CLANG_AST_LIB: ${CLANG_AST_LIB}")
message(STATUS "CLANG_FRONTEND_LIB: ${CLANG_FRONTEND_LIB}")
message(STATUS "CLANG_LEX_LIB: ${CLANG_LEX_LIB}")
message(STATUS "CLANG_PARSE_LIB: ${CLANG_PARSE_LIB}")
message(STATUS "CLANG_SEMA_LIB: ${CLANG_SEMA_LIB}")

# 如果没有找到Clang库，尝试在系统路径搜索
if(NOT CLANG_TOOLING_LIB)
    message(WARNING "在LLVM_LIBRARY_DIRS中没有找到clangTooling，尝试在系统路径中查找")
    find_library(CLANG_TOOLING_LIB NAMES clangTooling libclangTooling)
    find_library(CLANG_BASIC_LIB NAMES clangBasic libclangBasic)
    find_library(CLANG_AST_MATCHERS_LIB NAMES clangASTMatchers libclangASTMatchers)
    find_library(CLANG_AST_LIB NAMES clangAST libclangAST)
    find_library(CLANG_FRONTEND_LIB NAMES clangFrontend libclangFrontend)
    find_library(CLANG_LEX_LIB NAMES clangLex libclangLex)
    find_library(CLANG_PARSE_LIB NAMES clangParse libclangParse)
    find_library(CLANG_SEMA_LIB NAMES clangSema libclangSema)
endif()

# 再次检查是否找到了必要的库
if(NOT CLANG_TOOLING_LIB OR NOT CLANG_BASIC_LIB OR NOT CLANG_AST_LIB)
    message(FATAL_ERROR "未找到必要的Clang库，请安装clang-devel或libclang-dev软件包")
endif()

# 查找nlohmann_json库
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found via find_package, checking for header-only version")
    # 检查系统头文件目录中是否有nlohmann/json.hpp
    find_path(JSON_INCLUDE_DIR nlohmann/json.hpp
        PATHS
        /usr/include
        /usr/local/include
        /opt/include
        /opt/local/include
    )
    if(JSON_INCLUDE_DIR)
        message(STATUS "Found nlohmann/json.hpp in ${JSON_INCLUDE_DIR}")
        include_directories(${JSON_INCLUDE_DIR})
        set(NLOHMANN_JSON_FOUND TRUE)
    else()
        message(WARNING "nlohmann_json not found, some functionality may be limited")
        set(NLOHMANN_JSON_FOUND FALSE)
    endif()
else()
    message(STATUS "Found nlohmann_json: ${nlohmann_json_VERSION}")
    set(NLOHMANN_JSON_FOUND TRUE)
endif()

# 检查GoogleTest依赖
find_package(GTest QUIET)
if(NOT GTEST_FOUND)
    message(WARNING "GoogleTest not found, tests will not be built")
    set(BUILD_TESTS OFF)
else()
    message(STATUS "Found GoogleTest: ${GTEST_VERSION}")
    set(BUILD_TESTS ON)
endif()

# 设置源文件
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)

# 创建可执行文件
add_executable(dlogcover ${SOURCES})

# 链接依赖库
target_link_libraries(dlogcover
    PRIVATE
    # 使用找到的Clang库
    ${CLANG_TOOLING_LIB}
    ${CLANG_BASIC_LIB}
    ${CLANG_AST_MATCHERS_LIB}
    ${CLANG_AST_LIB}
    ${CLANG_FRONTEND_LIB}
    ${CLANG_LEX_LIB}
    ${CLANG_PARSE_LIB}
    ${CLANG_SEMA_LIB}
    LLVM
    # 添加对filesystem库的支持
    stdc++fs
)

# 如果找到了nlohmann_json，正确链接
if(nlohmann_json_FOUND)
    target_link_libraries(dlogcover PRIVATE nlohmann_json::nlohmann_json)
elseif(NLOHMANN_JSON_FOUND)
    # Header-only版本无需链接
endif()

# 构建单元测试
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 安装目标
install(TARGETS dlogcover DESTINATION bin)

# 生成编译命令数据库（用于代码分析工具）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 添加示例目录
add_subdirectory(examples)

# 安装规则
install(TARGETS dlogcover
    RUNTIME DESTINATION bin
)

# 安装示例配置文件
install(FILES examples/dlogcover.json
    DESTINATION share/dlogcover
)

# 包信息
set(CPACK_PACKAGE_NAME "dlogcover")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VENDOR "DLogCover Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C++ Log Coverage Analysis Tool")
set(CPACK_PACKAGE_DESCRIPTION "A tool for analyzing log coverage in C++ projects")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
include(CPack)
