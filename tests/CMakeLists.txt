# 测试目录的CMakeLists.txt

# 查找GoogleTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 配置单元测试

# 添加单元测试
add_subdirectory(unit)

# 添加集成测试（如果有）
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration)
    add_subdirectory(integration)
endif()

# 添加测试指令
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS unit_tests integration_tests
    COMMENT "Running all tests")

# 添加覆盖率目标
if(ENABLE_COVERAGE)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)

    if(LCOV_PATH AND GENHTML_PATH)
        message(STATUS "Found lcov: ${LCOV_PATH}")
        message(STATUS "Found genhtml: ${GENHTML_PATH}")

        # 添加覆盖率收集目标
        add_custom_target(coverage
            # 清除之前的覆盖率数据
            COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --zerocounters

            # 运行测试
            COMMAND ${CMAKE_CTEST_COMMAND} --verbose

            # 收集覆盖率数据
            COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --capture --output-file ${CMAKE_BINARY_DIR}/coverage.info

            # 过滤掉系统文件和测试文件
            COMMAND ${LCOV_PATH} --remove ${CMAKE_BINARY_DIR}/coverage.info '/usr/*' '*/tests/*' '*/third_party/*' --output-file ${CMAKE_BINARY_DIR}/coverage.filtered.info

            # 生成HTML报告
            COMMAND ${GENHTML_PATH} ${CMAKE_BINARY_DIR}/coverage.filtered.info --output-directory ${CMAKE_BINARY_DIR}/coverage_report

            # 显示报告摘要
            COMMAND ${LCOV_PATH} --summary ${CMAKE_BINARY_DIR}/coverage.filtered.info

            DEPENDS unit_tests
            COMMENT "Generating code coverage report"
        )

        # 打印覆盖率报告位置
        add_custom_command(TARGET coverage POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Coverage report generated at ${CMAKE_BINARY_DIR}/coverage_report/index.html"
        )
    else()
        message(WARNING "lcov or genhtml not found, cannot generate coverage reports")
    endif()
endif()
