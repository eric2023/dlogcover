cmake_minimum_required(VERSION 3.10)
project(dlogcover VERSION 0.1.0 LANGUAGES CXX)

# 添加cmake模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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
)

# 添加代码覆盖率选项 - 默认开启
option(ENABLE_COVERAGE "Enable coverage reporting" ON)

# 设置编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

    # 添加覆盖率标志（如果启用）
    if(ENABLE_COVERAGE)
        message(STATUS "Enabling code coverage support")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage -lgcov")
    endif()
endif()

# 检查LLVM依赖
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM_LIBRARY_DIRS: ${LLVM_LIBRARY_DIRS}")
message(STATUS "LLVM_INCLUDE_DIRS: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM_DEFINITIONS: ${LLVM_DEFINITIONS}")
# 映射必要的 LLVM 组件到库名
llvm_map_components_to_libnames(LLVM_LIB_LIST
    support
    core
    option
    irreader
    bitreader
    bitwriter
    nativecodegen
)
# 将组件库列表赋值给 LLVM_LIBRARIES
set(LLVM_LIBRARIES ${LLVM_LIB_LIST})
message(STATUS "LLVM component libraries: ${LLVM_LIBRARIES}")

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

# 查找新增的Clang库
find_library(CLANG_ANALYSIS_LIB clangAnalysis PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_DRIVER_LIB clangDriver PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_EDIT_LIB clangEdit PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_REWRITE_LIB clangRewrite PATHS ${LLVM_LIBRARY_DIRS})
find_library(CLANG_SERIALIZATION_LIB clangSerialization PATHS ${LLVM_LIBRARY_DIRS})

# 将clangOpenMP设置为可选库
find_library(CLANG_OPENMP_LIB clangOpenMP PATHS ${LLVM_LIBRARY_DIRS})
if(NOT CLANG_OPENMP_LIB)
    message(STATUS "clangOpenMP library not found - OpenMP support will be disabled")
endif()

# 显示找到的Clang库路径
message(STATUS "CLANG_TOOLING_LIB: ${CLANG_TOOLING_LIB}")
message(STATUS "CLANG_BASIC_LIB: ${CLANG_BASIC_LIB}")
message(STATUS "CLANG_AST_MATCHERS_LIB: ${CLANG_AST_MATCHERS_LIB}")
message(STATUS "CLANG_AST_LIB: ${CLANG_AST_LIB}")
message(STATUS "CLANG_FRONTEND_LIB: ${CLANG_FRONTEND_LIB}")
message(STATUS "CLANG_LEX_LIB: ${CLANG_LEX_LIB}")
message(STATUS "CLANG_PARSE_LIB: ${CLANG_PARSE_LIB}")
message(STATUS "CLANG_SEMA_LIB: ${CLANG_SEMA_LIB}")
message(STATUS "CLANG_ANALYSIS_LIB: ${CLANG_ANALYSIS_LIB}")
message(STATUS "CLANG_DRIVER_LIB: ${CLANG_DRIVER_LIB}")
message(STATUS "CLANG_EDIT_LIB: ${CLANG_EDIT_LIB}")
message(STATUS "CLANG_REWRITE_LIB: ${CLANG_REWRITE_LIB}")
message(STATUS "CLANG_SERIALIZATION_LIB: ${CLANG_SERIALIZATION_LIB}")
message(STATUS "CLANG_OPENMP_LIB: ${CLANG_OPENMP_LIB}")

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
    find_library(CLANG_ANALYSIS_LIB NAMES clangAnalysis libclangAnalysis)
    find_library(CLANG_DRIVER_LIB NAMES clangDriver libclangDriver)
    find_library(CLANG_EDIT_LIB NAMES clangEdit libclangEdit)
    find_library(CLANG_REWRITE_LIB NAMES clangRewrite libclangRewrite)
    find_library(CLANG_SERIALIZATION_LIB NAMES clangSerialization libclangSerialization)
    find_library(CLANG_OPENMP_LIB NAMES clangOpenMP libclangOpenMP)
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

# 检查GoogleTest依赖 - 始终设置构建测试为ON
find_package(GTest REQUIRED)
if(NOT GTEST_FOUND)
    message(FATAL_ERROR "GoogleTest not found, please install libgtest-dev package. Testing is required for this project.")
endif()
message(STATUS "Found GoogleTest: ${GTEST_VERSION}")
set(BUILD_TESTS ON)

# 设置测试覆盖率目标
set(TEST_SOURCES
    source_manager_test
    log_identifier_test
    ast_analyzer_test
    file_utils_test
    command_line_parser_test
    reporter_test
    coverage_calculator_test
    config_manager_test
)

# 设置源文件
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
)

# 创建可执行文件
add_executable(dlogcover ${SOURCES})

# 查找Clang依赖
find_package(Clang REQUIRED CONFIG)
message(STATUS "Found Clang ${CLANG_VERSION}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

# 链接依赖库
target_link_libraries(dlogcover
    PRIVATE
    clangTooling
    clangAST
    clangSema
    clangBasic
    LLVMSupport
    # 添加对filesystem库的支持
    stdc++fs
)

# 如果找到了nlohmann_json，正确链接
if(nlohmann_json_FOUND)
    target_link_libraries(dlogcover PRIVATE nlohmann_json::nlohmann_json)
elseif(NLOHMANN_JSON_FOUND)
    # Header-only版本无需链接
endif()

# 构建单元测试 - 确保主项目完成后构建测试
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)

    # 添加一个运行所有测试的目标
    add_custom_target(run_all_tests
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        DEPENDS ${TEST_SOURCES}
        COMMENT "Running all tests"
    )
endif()

# 创建一个按顺序执行构建、测试和覆盖率报告的目标
if(BUILD_TESTS AND ENABLE_COVERAGE)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    if(LCOV_PATH AND GENHTML_PATH)
        # 确保CheckCoverage.cmake存在
        if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckCoverage.cmake")
            message(FATAL_ERROR "CheckCoverage.cmake not found in ${CMAKE_CURRENT_SOURCE_DIR}/cmake")
        endif()

        # 添加一个按步骤执行的目标
        add_custom_target(build_test_coverage
            # 首先确保主项目编译成功
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target dlogcover
            # 构建测试项目
            COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target run_all_tests
            # 清除之前的覆盖率数据
            COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --zerocounters
            # 运行测试
            COMMAND ${CMAKE_CTEST_COMMAND} --verbose
            # 收集覆盖率数据
            COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --capture --output-file ${CMAKE_BINARY_DIR}/coverage.info
            # 过滤掉系统文件和测试文件
            COMMAND ${LCOV_PATH} --remove ${CMAKE_BINARY_DIR}/coverage.info '/usr/*' '*/tests/*' --output-file ${CMAKE_BINARY_DIR}/coverage.filtered.info
            # 生成HTML报告
            COMMAND ${GENHTML_PATH} ${CMAKE_BINARY_DIR}/coverage.filtered.info --output-directory ${CMAKE_BINARY_DIR}/coverage_report
            # 显示报告摘要
            COMMAND ${LCOV_PATH} --summary ${CMAKE_BINARY_DIR}/coverage.filtered.info
            # 检查覆盖率是否达到要求
            COMMAND ${CMAKE_COMMAND} -DCMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR} -DCMAKE_BINARY_DIR=${CMAKE_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckCoverage.cmake

            DEPENDS dlogcover ${TEST_SOURCES}
            COMMENT "执行完整的测试流程：1.编译项目 -> 2.运行测试 -> 3.生成覆盖率报告 -> 4.检查覆盖率要求"
        )

        # 打印覆盖率报告位置
        add_custom_command(TARGET build_test_coverage POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "覆盖率报告已生成：${CMAKE_BINARY_DIR}/coverage_report/index.html"
            COMMAND ${CMAKE_COMMAND} -E echo "请确保覆盖率满足以下要求："
            COMMAND ${CMAKE_COMMAND} -E echo "  - 单元测试覆盖率 > 80%"
            COMMAND ${CMAKE_COMMAND} -E echo "  - 关键模块覆盖率 > 90%"
            COMMAND ${CMAKE_COMMAND} -E echo "  - 集成测试场景覆盖率 > 85%"
        )
    else()
        message(FATAL_ERROR "lcov或genhtml未找到，无法生成覆盖率报告。请安装lcov包。")
    endif()
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
