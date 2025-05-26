/**
 * @file ast_types.h
 * @brief AST分析器公共类型定义
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_AST_ANALYZER_AST_TYPES_H
#define DLOGCOVER_CORE_AST_ANALYZER_AST_TYPES_H

#include <dlogcover/common/result.h>
#include <string>
#include <vector>
#include <memory>

namespace dlogcover {
namespace core {
namespace ast_analyzer {

/**
 * @brief AST分析器错误类型
 */
enum class ASTAnalyzerError {
    FILE_NOT_FOUND = 1,         ///< 文件未找到
    FILE_READ_ERROR = 2,        ///< 文件读取错误
    COMPILATION_ERROR = 3,      ///< 编译错误
    INTERNAL_ERROR = 4,         ///< 内部错误
    INVALID_AST = 5             ///< 无效的AST
};

/**
 * @brief AST节点类型
 */
enum class NodeType {
    FUNCTION = 0,               ///< 函数节点
    METHOD = 1,                 ///< 方法节点
    BRANCH = 2,                 ///< 分支节点
    LOOP = 3,                   ///< 循环节点
    EXPRESSION = 4,             ///< 表达式节点
    STATEMENT = 5,              ///< 语句节点
    DECLARATION = 6,            ///< 声明节点
    TRY_CATCH = 7,              ///< 异常处理节点
    CALL_EXPR = 8,              ///< 函数调用表达式
    LOG_CALL_EXPR = 9           ///< 日志函数调用表达式
};

/**
 * @brief 位置信息结构
 */
struct LocationInfo {
    std::string filePath;       ///< 文件路径
    int line;                   ///< 行号
    int column;                 ///< 列号
    int endLine;                ///< 结束行号
    int endColumn;              ///< 结束列号
};

/**
 * @brief AST节点信息结构
 */
struct ASTNodeInfo {
    NodeType type;              ///< 节点类型
    std::string name;           ///< 节点名称
    std::string text;           ///< 源码文本
    LocationInfo location;      ///< 位置信息
    bool hasLogging;            ///< 是否包含日志记录
    std::vector<std::unique_ptr<ASTNodeInfo>> children;  ///< 子节点列表
    
    /**
     * @brief 默认构造函数
     */
    ASTNodeInfo() : type(NodeType::STATEMENT), hasLogging(false) {}
    
    /**
     * @brief 移动构造函数
     */
    ASTNodeInfo(ASTNodeInfo&&) = default;
    
    /**
     * @brief 移动赋值运算符
     */
    ASTNodeInfo& operator=(ASTNodeInfo&&) = default;
    
    // 禁用复制构造函数和复制赋值运算符
    ASTNodeInfo(const ASTNodeInfo&) = delete;
    ASTNodeInfo& operator=(const ASTNodeInfo&) = delete;
};

/**
 * @brief 结果类型别名
 */
template<typename T>
using Result = dlogcover::Result<T, ASTAnalyzerError>;

/**
 * @brief 创建成功结果的辅助函数
 */
template<typename T>
Result<T> makeSuccess(T&& value) {
    return Result<T>(std::forward<T>(value));
}

/**
 * @brief 创建错误结果的辅助函数
 */
template<typename T>
Result<T> makeError(ASTAnalyzerError error, const std::string& message) {
    return Result<T>(error, message);
}

} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_AST_ANALYZER_AST_TYPES_H 