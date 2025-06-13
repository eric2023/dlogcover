# 集成测试的CMakeLists.txt

# 设置集成测试源文件列表
set(INTEGRATION_TEST_SOURCES
    config_workflow_test.cpp
    log_analysis_test.cpp
    coverage_workflow_test.cpp
    error_handling_test.cpp
)

# 添加测试工具类源文件（测试专用，不在静态库中）
set(TEST_COMMON_SOURCES
    "${CMAKE_SOURCE_DIR}/tests/common/test_utils.cpp"
)

# 创建集成测试可执行文件
foreach(TEST_SOURCE ${INTEGRATION_TEST_SOURCES})
    # 获取文件名（不含扩展名）作为目标名
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    # 创建测试可执行文件 - 使用静态库
    add_executable(${TEST_NAME} ${TEST_SOURCE} ${TEST_COMMON_SOURCES})

    # 包含头文件目录
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/common
    )

    # 链接依赖库 - 使用静态库简化依赖
    target_link_libraries(${TEST_NAME}
        PRIVATE
        dlogcover_lib  # 使用静态库，包含所有核心依赖
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
        pthread
    )

    # 添加依赖关系，确保先构建静态库
    add_dependencies(${TEST_NAME} dlogcover_lib)

    # 添加到CTest
    add_test(NAME "integration_${TEST_NAME}" COMMAND ${TEST_NAME})

    # 设置测试属性
    set_tests_properties("integration_${TEST_NAME}" PROPERTIES
        ENVIRONMENT "GTEST_COLOR=1"
        TIMEOUT 600  # 集成测试可能需要更长时间
    )
endforeach()

# 创建一个运行所有集成测试的目标
add_custom_target(integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -R "^integration_" --output-on-failure
    DEPENDS ${INTEGRATION_TEST_SOURCES}
    COMMENT "运行所有集成测试"
)
