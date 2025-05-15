# 单元测试的CMakeLists.txt

# 设置源文件
file(GLOB UNIT_TEST_SOURCES "*.cpp")

# 收集项目源文件（排除main.cpp）
file(GLOB_RECURSE PROJECT_SOURCES
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

# 创建单元测试可执行文件
add_executable(unit_tests ${UNIT_TEST_SOURCES} ${PROJECT_SOURCES})

# 包含头文件目录
target_include_directories(unit_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 链接依赖库
target_link_libraries(unit_tests
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
    target_link_libraries(unit_tests PRIVATE nlohmann_json::nlohmann_json)
endif()

# 添加测试
add_test(NAME unit_tests COMMAND unit_tests)
