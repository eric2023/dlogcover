cmake_minimum_required(VERSION 3.10)
project(dlogcover_cmd_test VERSION 0.1.0 LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加代码覆盖率支持
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 查找GoogleTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 设置源文件
set(CLI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cli/command_line_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/cli/options.cpp
)

set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/unit/command_line_parser_test.cpp
)

# 创建可执行文件
add_executable(cmd_parser_test ${TEST_SOURCES} ${CLI_SOURCES})

# 链接依赖库
target_link_libraries(cmd_parser_test
    PRIVATE
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
)

# 添加测试
add_test(NAME cmd_parser_test COMMAND cmd_parser_test)

# 添加覆盖率目标
find_program(GCOV_PATH gcov)
find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)

if(GCOV_PATH AND LCOV_PATH AND GENHTML_PATH)
    add_custom_target(coverage
        # 清除之前的覆盖率数据
        COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --zerocounters

        # 运行测试
        COMMAND ./cmd_parser_test

        # 收集覆盖率数据
        COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --capture --output-file ${CMAKE_BINARY_DIR}/coverage.info

        # 过滤掉系统文件和测试文件
        COMMAND ${LCOV_PATH} --remove ${CMAKE_BINARY_DIR}/coverage.info '/usr/*' '*/tests/*' '*/third_party/*' --output-file ${CMAKE_BINARY_DIR}/coverage.filtered.info

        # 生成HTML报告
        COMMAND ${GENHTML_PATH} ${CMAKE_BINARY_DIR}/coverage.filtered.info --output-directory ${CMAKE_BINARY_DIR}/coverage_report

        # 显示报告摘要
        COMMAND ${LCOV_PATH} --summary ${CMAKE_BINARY_DIR}/coverage.filtered.info

        DEPENDS cmd_parser_test
        COMMENT "Generating code coverage report"
    )

    # 打印覆盖率报告位置
    add_custom_command(TARGET coverage POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated at ${CMAKE_BINARY_DIR}/coverage_report/index.html"
    )
else()
    message(WARNING "gcov, lcov or genhtml not found, cannot generate coverage reports")
endif()
