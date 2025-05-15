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
