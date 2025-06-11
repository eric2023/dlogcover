/**
 * @file ast_types.h
 * @brief AST分析相关的类型定义
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_AST_ANALYZER_AST_TYPES_H
#define DLOGCOVER_CORE_AST_ANALYZER_AST_TYPES_H

#include <dlogcover/common/result.h>
#include <string>
#include <memory>
#include <vector>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

/**
 * @brief AST分析器错误类型
 */
enum class ASTAnalyzerError {
    NONE = 0,                      ///< 无错误
    FILE_NOT_FOUND,                ///< 文件未找到
    FILE_READ_ERROR,               ///< 文件读取错误
    PARSE_ERROR,                   ///< 解析错误
    ANALYSIS_ERROR,                ///< 分析错误
    CLANG_ERROR,                   ///< Clang错误
    MEMORY_ERROR,                  ///< 内存错误
    COMPILATION_ERROR,             ///< 编译错误
    INTERNAL_ERROR,                ///< 内部错误
    INVALID_AST                    ///< 无效的AST
};

/**
 * @brief AST节点类型枚举
 */
enum class NodeType {
    UNKNOWN = 0,                   ///< 未知类型
    FUNCTION = 1,                  ///< 函数节点
    METHOD = 2,                    ///< 方法节点
    FUNCTION_DECL,                 ///< 函数声明
    CALL_EXPR,                     ///< 函数调用表达式
    LOG_CALL_EXPR,                 ///< 日志函数调用表达式
    VARIABLE_DECL,                 ///< 变量声明
    COMPOUND_STMT,                 ///< 复合语句
    IF_STMT,                       ///< if语句
    ELSE_STMT,                     ///< else语句
    SWITCH_STMT,                   ///< switch语句
    CASE_STMT,                     ///< case语句
    FOR_STMT,                      ///< for语句
    WHILE_STMT,                    ///< while语句
    DO_STMT,                       ///< do语句
    TRY_STMT,                      ///< try语句
    CATCH_STMT,                    ///< catch语句
    LOG_CALL,                      ///< 日志调用
    BRANCH,                        ///< 分支节点
    LOOP,                          ///< 循环节点
    EXPRESSION,                    ///< 表达式节点
    STATEMENT,                     ///< 语句节点
    DECLARATION,                   ///< 声明节点
    TRY_CATCH                      ///< 异常处理节点
};

/**
 * @brief 位置信息结构体
 */
struct LocationInfo {
    std::string filePath;          ///< 文件路径
    std::string fileName;          ///< 文件名
    unsigned int line = 0;         ///< 行号
    unsigned int column = 0;       ///< 列号
    int endLine = 0;               ///< 结束行号
    int endColumn = 0;             ///< 结束列号
    
    /**
     * @brief 默认构造函数
     */
    LocationInfo() = default;
    
    /**
     * @brief 构造函数
     * @param file 文件名
     * @param ln 行号
     * @param col 列号
     */
    LocationInfo(const std::string& file, unsigned int ln, unsigned int col)
        : fileName(file), line(ln), column(col) {}
};

// 为了与现有cpp文件兼容，提供Location别名
using Location = LocationInfo;

/**
 * @brief AST节点信息结构体
 */
struct ASTNodeInfo {
    NodeType type = NodeType::UNKNOWN;     ///< 节点类型
    std::string name;                      ///< 节点名称
    LocationInfo location;                 ///< 位置信息
    LocationInfo endLocation;              ///< 结束位置信息
    std::string text;                      ///< 源码文本
    bool hasLogging = false;               ///< 是否包含日志记录
    std::vector<std::unique_ptr<ASTNodeInfo>> children;  ///< 子节点
    
    /**
     * @brief 默认构造函数
     */
    ASTNodeInfo() = default;
    
    /**
     * @brief 构造函数
     * @param t 节点类型
     * @param n 节点名称
     * @param loc 位置信息
     */
    ASTNodeInfo(NodeType t, const std::string& n, const LocationInfo& loc)
        : type(t), name(n), location(loc) {}

    /**
     * @brief 移动构造函数
     */
    ASTNodeInfo(ASTNodeInfo&&) = default;
    
    /**
     * @brief 移动赋值运算符
     */
    ASTNodeInfo& operator=(ASTNodeInfo&&) = default;
    
    /**
     * @brief 拷贝构造函数（用于缓存深拷贝）
     */
    ASTNodeInfo(const ASTNodeInfo& other);
    
    /**
     * @brief 拷贝赋值运算符
     */
    ASTNodeInfo& operator=(const ASTNodeInfo& other);
};

// Result类型别名
template<typename T>
using Result = dlogcover::Result<T, ASTAnalyzerError>;

// 常用的Result类型别名
using NodeResult = Result<std::unique_ptr<ASTNodeInfo>>;
using BoolResult = Result<bool>;

/**
 * @brief 创建成功结果的辅助函数
 */
template<typename T>
Result<T> makeSuccess(T&& value) {
    return dlogcover::makeSuccess<T, ASTAnalyzerError>(std::forward<T>(value));
}

/**
 * @brief 创建错误结果的辅助函数
 */
template<typename T>
Result<T> makeError(ASTAnalyzerError error, const std::string& message) {
    return dlogcover::makeError<T, ASTAnalyzerError>(error, message);
}

} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_AST_ANALYZER_AST_TYPES_H 