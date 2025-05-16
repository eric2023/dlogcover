# 检查覆盖率是否达到要求的CMake脚本

# 读取覆盖率报告
file(READ "${CMAKE_BINARY_DIR}/coverage.filtered.info" COVERAGE_DATA)

# 解析覆盖率数据
string(REGEX MATCH "lines\\.*: ([0-9.]+)%" LINES_COVERAGE ${COVERAGE_DATA})
set(TOTAL_COVERAGE ${CMAKE_MATCH_1})

# 设置覆盖率要求
set(MINIMUM_COVERAGE 80.0)
set(CRITICAL_MODULE_COVERAGE 90.0)
set(INTEGRATION_COVERAGE 85.0)

# 检查总体覆盖率
if(${TOTAL_COVERAGE} LESS ${MINIMUM_COVERAGE})
    message(FATAL_ERROR "总体代码覆盖率 (${TOTAL_COVERAGE}%) 低于要求的最小覆盖率 (${MINIMUM_COVERAGE}%)")
endif()

# 检查关键模块覆盖率
foreach(MODULE IN ITEMS
    "src/ast_analyzer"
    "src/log_identifier"
    "src/coverage_calculator")

    string(REGEX MATCH "${MODULE}.*lines\\.*: ([0-9.]+)%" MODULE_COVERAGE ${COVERAGE_DATA})
    if(${CMAKE_MATCH_1} LESS ${CRITICAL_MODULE_COVERAGE})
        message(FATAL_ERROR "关键模块 ${MODULE} 的覆盖率 (${CMAKE_MATCH_1}%) 低于要求 (${CRITICAL_MODULE_COVERAGE}%)")
    endif()
endforeach()

# 检查集成测试覆盖率
string(REGEX MATCH "tests/integration.*lines\\.*: ([0-9.]+)%" INTEGRATION_COV ${COVERAGE_DATA})
if(${CMAKE_MATCH_1} LESS ${INTEGRATION_COVERAGE})
    message(FATAL_ERROR "集成测试覆盖率 (${CMAKE_MATCH_1}%) 低于要求 (${INTEGRATION_COVERAGE}%)")
endif()

message(STATUS "覆盖率检查通过：")
message(STATUS "  - 总体覆盖率: ${TOTAL_COVERAGE}% (要求 > ${MINIMUM_COVERAGE}%)")
message(STATUS "  - 关键模块覆盖率: 所有模块 > ${CRITICAL_MODULE_COVERAGE}%")
message(STATUS "  - 集成测试覆盖率: ${CMAKE_MATCH_1}% (要求 > ${INTEGRATION_COVERAGE}%)")
