# 单元测试的CMakeLists.txt

# 设置测试源文件列表
set(TEST_SOURCES
    source_manager_test.cpp
    log_identifier_test.cpp
    ast_analyzer_test.cpp
    file_utils_test.cpp
    command_line_parser_test.cpp
    reporter_test.cpp
    coverage_calculator_test.cpp
    config_manager_test.cpp
    cmake_parser_test.cpp
    file_ownership_validator_test.cpp
    path_normalizer_test.cpp
)

# 收集项目源文件（排除main.cpp）
file(GLOB_RECURSE PROJECT_SOURCES
    "${CMAKE_SOURCE_DIR}/src/common/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/config/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/source_manager/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/cli/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/ast_analyzer/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/log_identifier/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/coverage/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/reporter/*.cpp"
)
# 排除main.cpp
list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# 为每个测试文件创建单独的目标
foreach(TEST_SOURCE ${TEST_SOURCES})
    # 获取文件名（不含扩展名）作为目标名
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

    # 创建测试可执行文件
    add_executable(${TEST_NAME} ${TEST_SOURCE} ${PROJECT_SOURCES})

# 包含头文件目录
    target_include_directories(${TEST_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 链接依赖库
    target_link_libraries(${TEST_NAME}
    PRIVATE
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
    stdc++fs
    # 添加Clang/LLVM库
    clangTooling
    clangBasic
    clangASTMatchers
    clangAST
    clangFrontend
    clangLex
    clangParse
    clangSema
    LLVM
)

# 如果找到了nlohmann_json的CMake模块，使用它
if(nlohmann_json_FOUND)
        target_link_libraries(${TEST_NAME} PRIVATE nlohmann_json::nlohmann_json)
endif()

# 添加依赖关系，确保先构建主项目
    add_dependencies(${TEST_NAME} dlogcover)

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
