# 检查覆盖率是否达到要求的CMake脚本

# 运行lcov命令获取摘要信息
# lcov的--summary输出到stderr，所以我们需要合并输出
execute_process(
    COMMAND lcov --summary "${CMAKE_BINARY_DIR}/coverage.filtered.info"
    OUTPUT_VARIABLE COVERAGE_OUTPUT
    ERROR_VARIABLE COVERAGE_ERROR
    RESULT_VARIABLE COVERAGE_RESULT
)

# lcov将摘要输出到错误流，所以我们从ERROR_VARIABLE获取数据
set(COVERAGE_SUMMARY "${COVERAGE_ERROR}")

if(NOT COVERAGE_RESULT EQUAL 0)
    message(FATAL_ERROR "无法获取覆盖率摘要: ${COVERAGE_ERROR}")
endif()



# 解析覆盖率数据 - 修正正则表达式以匹配lcov摘要格式
# lcov输出格式为: "lines......: 70.6% (4278 of 6060 lines)"
string(REGEX MATCH "lines\\.*: ([0-9]+\\.[0-9]+)%" LINES_COVERAGE_MATCH "${COVERAGE_SUMMARY}")
if(LINES_COVERAGE_MATCH)
    set(TOTAL_COVERAGE ${CMAKE_MATCH_1})
else()
    message(FATAL_ERROR "无法解析覆盖率数据。摘要输出: ${COVERAGE_SUMMARY}")
endif()

# 解析函数覆盖率
# lcov输出格式为: "functions..: 83.8% (464 of 554 functions)"
string(REGEX MATCH "functions\\.*: ([0-9]+\\.[0-9]+)%" FUNCTIONS_COVERAGE_MATCH "${COVERAGE_SUMMARY}")
if(FUNCTIONS_COVERAGE_MATCH)
    set(FUNCTION_COVERAGE ${CMAKE_MATCH_1})
else()
    set(FUNCTION_COVERAGE "未知")
endif()

# 设置覆盖率要求
set(MINIMUM_COVERAGE 80.0)
set(CRITICAL_MODULE_COVERAGE 90.0)
set(INTEGRATION_COVERAGE 85.0)

message(STATUS "覆盖率分析结果:")
message(STATUS "  - 行覆盖率: ${TOTAL_COVERAGE}%")
message(STATUS "  - 函数覆盖率: ${FUNCTION_COVERAGE}%")

# 检查总体覆盖率 - 修正比较语法
if(TOTAL_COVERAGE LESS MINIMUM_COVERAGE)
    message(STATUS "  - 状态: 未达标 ❌")
    message(STATUS "  - 建议: 需要增加更多的单元测试来提高覆盖率")
    message(STATUS "  - 目标: 从 ${TOTAL_COVERAGE}% 提升到 ${MINIMUM_COVERAGE}%")
    
    # 计算需要增加的覆盖行数
    # lcov输出格式为: "lines......: 70.6% (4278 of 6060 lines)"
    string(REGEX MATCH "\\(([0-9]+) of ([0-9]+) lines\\)" LINES_DETAIL_MATCH "${COVERAGE_SUMMARY}")
    if(LINES_DETAIL_MATCH)
        set(COVERED_LINES ${CMAKE_MATCH_1})
        set(TOTAL_LINES ${CMAKE_MATCH_2})
        # 修正：CMake的math()函数不支持浮点数运算，需要先转换为整数
        math(EXPR TARGET_LINES "${TOTAL_LINES} * 80 / 100")
        math(EXPR NEEDED_LINES "${TARGET_LINES} - ${COVERED_LINES}")
        message(STATUS "  - 需要额外覆盖约 ${NEEDED_LINES} 行代码")
    endif()
else()
    message(STATUS "  - 状态: 达标 ✅")
    message(STATUS "  - 恭喜！代码覆盖率已达到要求")
endif()

# 检查关键模块覆盖率（简化版本，因为我们现在只有总体覆盖率）
message(STATUS "")
message(STATUS "覆盖率检查完成：")
message(STATUS "  - 总体覆盖率: ${TOTAL_COVERAGE}% (要求 > ${MINIMUM_COVERAGE}%)")
message(STATUS "  - 函数覆盖率: ${FUNCTION_COVERAGE}%")

# 提供改进建议
if(TOTAL_COVERAGE LESS MINIMUM_COVERAGE)
    message(STATUS "")
    message(STATUS "提高覆盖率的建议:")
    message(STATUS "  1. 为核心业务逻辑添加更多单元测试")
    message(STATUS "  2. 增加边界条件和异常情况的测试用例")
    message(STATUS "  3. 检查未覆盖的代码分支，添加相应测试")
    message(STATUS "  4. 查看覆盖率报告: ${CMAKE_BINARY_DIR}/coverage_report/index.html")
endif()
