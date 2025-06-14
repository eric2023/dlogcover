# 单元测试的CMakeLists.txt

# 添加测试工具类源文件（测试专用，不在静态库中）
set(TEST_COMMON_SOURCES
    "${CMAKE_SOURCE_DIR}/tests/common/test_utils.cpp"
)

# 设置测试源文件列表
set(TEST_SOURCES
    source_manager_test.cpp
    log_identifier_test.cpp
    log_identifier_edge_cases_test.cpp
    log_identifier_simple_test.cpp
    ast_analyzer_test.cpp
    ast_analyzer_parallel_test.cpp
    ast_cache_time_test.cpp
    file_utils_test.cpp
    command_line_parser_test.cpp
    reporter_test.cpp
    coverage_calculator_test.cpp
    config_manager_test.cpp
    cmake_parser_test.cpp
    file_ownership_validator_test.cpp
    path_normalizer_test.cpp
    result_test.cpp
    # 新增的扩展测试文件
    compile_commands_manager_test.cpp
    options_extended_test.cpp
    string_utils_extended_test.cpp
    log_types_test.cpp
    config_validator_extended_test.cpp
    command_line_parser_extended_test.cpp
)

# 添加根目录下的测试文件
set(ROOT_TEST_SOURCES
    "${CMAKE_SOURCE_DIR}/tests/thread_pool_test.cpp"
    "${CMAKE_SOURCE_DIR}/tests/ast_cache_test.cpp"
)

# 为根目录测试文件创建单独的目标
foreach(ROOT_TEST_SOURCE ${ROOT_TEST_SOURCES})
    # 获取文件名（不含扩展名）作为目标名
    get_filename_component(ROOT_TEST_NAME ${ROOT_TEST_SOURCE} NAME_WE)

    # 创建测试可执行文件 - 使用静态库
    add_executable(${ROOT_TEST_NAME} ${ROOT_TEST_SOURCE} ${TEST_COMMON_SOURCES})

    # 包含头文件目录
    target_include_directories(${ROOT_TEST_NAME} PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/common
    )

    # 链接依赖库 - 使用静态库简化依赖
    target_link_libraries(${ROOT_TEST_NAME}
        PRIVATE
        dlogcover_lib  # 使用静态库，包含所有核心依赖
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
        pthread
    )

    # 添加依赖关系，确保先构建静态库
    add_dependencies(${ROOT_TEST_NAME} dlogcover_lib)

    # 添加到CTest
    add_test(NAME ${ROOT_TEST_NAME} COMMAND ${ROOT_TEST_NAME})

    # 设置测试属性
    set_tests_properties(${ROOT_TEST_NAME} PROPERTIES
        ENVIRONMENT "GTEST_COLOR=1"
        TIMEOUT 300)
endforeach()

# 为每个测试文件创建单独的目标
foreach(TEST_SOURCE ${TEST_SOURCES})
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
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

    # 设置测试属性
    set_tests_properties(${TEST_NAME} PROPERTIES
        ENVIRONMENT "GTEST_COLOR=1"
        TIMEOUT 300)
endforeach()

# 创建一个运行所有单元测试的目标
add_custom_target(unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${TEST_SOURCES}
    COMMENT "运行所有单元测试"
)
