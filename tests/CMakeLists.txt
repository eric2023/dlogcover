# 测试目录的CMakeLists.txt

# 查找GoogleTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 添加测试工具类源文件（测试专用，不在静态库中）
set(TEST_COMMON_SOURCES
    "${CMAKE_SOURCE_DIR}/tests/common/test_utils.cpp"
)

# 添加死锁检测测试
add_executable(deadlock_detection_test
    deadlock_detection_test.cpp
    ${TEST_COMMON_SOURCES}
)

target_link_libraries(deadlock_detection_test
    PRIVATE
    dlogcover_lib  # 使用静态库，包含所有核心依赖
    ${GTEST_LIBRARIES}
    gtest_main
    pthread
)

target_include_directories(deadlock_detection_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests/common
)

# 添加并发安全测试
add_executable(concurrent_safety_test
    concurrent_safety_test.cpp
    ${TEST_COMMON_SOURCES}
)

target_link_libraries(concurrent_safety_test
    PRIVATE
    dlogcover_lib  # 使用静态库，包含所有核心依赖
    ${GTEST_LIBRARIES}
    gtest_main
    pthread
)

target_include_directories(concurrent_safety_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tests/common
)

# 确保所有测试依赖于静态库
add_dependencies(deadlock_detection_test dlogcover_lib)
add_dependencies(concurrent_safety_test dlogcover_lib)

# 注册测试
add_test(NAME DeadlockDetectionTest COMMAND deadlock_detection_test)
add_test(NAME ConcurrentSafetyTest COMMAND concurrent_safety_test)

# 配置单元测试
add_subdirectory(unit)

# 添加集成测试（如果有）
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/integration)
    add_subdirectory(integration)
endif()

# 添加测试指令，确保依赖于静态库
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS dlogcover_lib ${TEST_SOURCES}
    COMMENT "运行所有测试")

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
            COMMAND ${LCOV_PATH} --remove ${CMAKE_BINARY_DIR}/coverage.info '/usr/*' '*/tests/*' --output-file ${CMAKE_BINARY_DIR}/coverage.filtered.info

            # 生成HTML报告
            COMMAND ${GENHTML_PATH} ${CMAKE_BINARY_DIR}/coverage.filtered.info --output-directory ${CMAKE_BINARY_DIR}/coverage_report

            # 显示报告摘要
            COMMAND ${LCOV_PATH} --summary ${CMAKE_BINARY_DIR}/coverage.filtered.info

            DEPENDS dlogcover_lib ${TEST_SOURCES}
            COMMENT "生成代码覆盖率报告"
        )

        # 打印覆盖率报告位置
        add_custom_command(TARGET coverage POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "覆盖率报告生成在 ${CMAKE_BINARY_DIR}/coverage_report/index.html"
        )
    else()
        message(WARNING "lcov或genhtml未找到，无法生成覆盖率报告")
    endif()
endif()
