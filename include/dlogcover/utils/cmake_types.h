/**
 * @file cmake_types.h
 * @brief CMake解析相关的类型定义
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_UTILS_CMAKE_TYPES_H
#define DLOGCOVER_UTILS_CMAKE_TYPES_H

#include <string>
#include <vector>
#include <unordered_map>

namespace dlogcover {
namespace utils {

/**
 * @brief CMake解析错误类型
 */
enum class CMakeParserError {
    NONE = 0,                      ///< 无错误
    FILE_NOT_FOUND,                ///< CMakeLists.txt文件未找到
    FILE_READ_ERROR,               ///< 文件读取错误
    PARSE_ERROR,                   ///< 解析错误
    INVALID_SYNTAX,                ///< 无效语法
    VARIABLE_NOT_FOUND,            ///< 变量未找到
    INTERNAL_ERROR                 ///< 内部错误
};

/**
 * @brief CMake变量类型
 */
enum class CMakeVariableType {
    STRING,                        ///< 字符串变量
    LIST,                          ///< 列表变量
    BOOLEAN,                       ///< 布尔变量
    PATH,                          ///< 路径变量
    CACHE                          ///< 缓存变量
};

/**
 * @brief CMake变量信息
 */
struct CMakeVariable {
    std::string name;              ///< 变量名
    std::string value;             ///< 变量值
    CMakeVariableType type;        ///< 变量类型
    std::string description;       ///< 变量描述
    bool isCache;                  ///< 是否为缓存变量
    
    CMakeVariable() : type(CMakeVariableType::STRING), isCache(false) {}
    
    CMakeVariable(const std::string& n, const std::string& v, 
                  CMakeVariableType t = CMakeVariableType::STRING, 
                  bool cache = false)
        : name(n), value(v), type(t), isCache(cache) {}
};

/**
 * @brief CMake目标信息
 */
struct CMakeTarget {
    std::string name;              ///< 目标名称
    std::string type;              ///< 目标类型（EXECUTABLE, LIBRARY等）
    std::vector<std::string> sources;        ///< 源文件列表
    std::vector<std::string> includeDirectories;  ///< 包含目录
    std::vector<std::string> compileDefinitions;  ///< 编译定义
    std::vector<std::string> compileOptions;      ///< 编译选项
    std::vector<std::string> linkLibraries;       ///< 链接库
    
    CMakeTarget() = default;
    CMakeTarget(const std::string& n, const std::string& t) : name(n), type(t) {}
};

/**
 * @brief CMake解析结果
 */
struct CMakeParseResult {
    // 基本信息
    std::string projectName;       ///< 项目名称
    std::string projectVersion;    ///< 项目版本
    std::string cxxStandard;       ///< C++标准
    std::string cStandard;         ///< C标准
    
    // 编译相关
    std::vector<std::string> includeDirectories;    ///< 全局包含目录
    std::vector<std::string> compileDefinitions;    ///< 全局编译定义
    std::vector<std::string> compileOptions;        ///< 全局编译选项
    std::vector<std::string> linkDirectories;       ///< 链接目录
    std::vector<std::string> linkLibraries;         ///< 链接库
    
    // 变量和目标
    std::unordered_map<std::string, CMakeVariable> variables;  ///< 变量映射
    std::unordered_map<std::string, CMakeTarget> targets;      ///< 目标映射
    
    // 路径信息
    std::string sourceDir;         ///< 源码目录
    std::string binaryDir;         ///< 构建目录
    std::string cmakeListsPath;    ///< CMakeLists.txt路径
    
    CMakeParseResult() = default;
    
    /**
     * @brief 获取所有编译参数
     * @return 编译参数列表
     */
    std::vector<std::string> getAllCompilerArgs() const;
    
    /**
     * @brief 获取指定目标的编译参数
     * @param targetName 目标名称
     * @return 编译参数列表
     */
    std::vector<std::string> getTargetCompilerArgs(const std::string& targetName) const;
    
    /**
     * @brief 检查是否为有效的解析结果
     * @return 如果有效返回true
     */
    bool isValid() const;
    
    /**
     * @brief 清空所有数据
     */
    void clear();
};

} // namespace utils
} // namespace dlogcover

#endif // DLOGCOVER_UTILS_CMAKE_TYPES_H 