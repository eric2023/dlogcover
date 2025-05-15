# 集成测试的CMakeLists.txt

# 设置源文件
file(GLOB INTEGRATION_TEST_SOURCES "*.cpp")

# 创建集成测试可执行文件
add_executable(integration_tests ${INTEGRATION_TEST_SOURCES})

# 链接依赖库
target_link_libraries(integration_tests
    PRIVATE
    ${GTEST_LIBRARIES}
    ${GTEST_MAIN_LIBRARIES}
    pthread
)

# 添加测试
add_test(NAME integration_tests COMMAND integration_tests) 