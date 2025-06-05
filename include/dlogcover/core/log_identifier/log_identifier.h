/**
 * @file log_identifier.h
 * @brief 日志识别器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_LOG_IDENTIFIER_LOG_IDENTIFIER_H
#define DLOGCOVER_CORE_LOG_IDENTIFIER_LOG_IDENTIFIER_H

#include <dlogcover/core/log_identifier/identifier_types.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/config/config.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace dlogcover {
namespace core {
namespace log_identifier {

/**
 * @brief 日志识别器类
 * 
 * 负责识别源代码中的日志调用
 */
class LogIdentifier {
public:
    /**
     * @brief 构造函数
     * @param config 配置对象引用
     * @param astAnalyzer AST分析器引用
     */
    LogIdentifier(const config::Config& config, const ast_analyzer::ASTAnalyzer& astAnalyzer);

    /**
     * @brief 析构函数
     */
    ~LogIdentifier();

    /**
     * @brief 识别所有文件中的日志调用
     * @return 识别结果，成功返回true，否则返回错误信息
     */
    ast_analyzer::Result<bool> identifyLogCalls();

    /**
     * @brief 获取指定文件的日志调用信息
     * @param filePath 文件路径
     * @return 日志调用信息列表的常量引用
     */
    const std::vector<LogCallInfo>& getLogCalls(const std::string& filePath) const;

    /**
     * @brief 获取所有文件的日志调用信息
     * @return 所有文件的日志调用信息映射表的常量引用
     */
    const std::unordered_map<std::string, std::vector<LogCallInfo>>& getAllLogCalls() const;

    /**
     * @brief 获取日志类型
     * @param functionName 函数名
     * @return 日志类型
     */
    LogType getLogType(const std::string& functionName) const;

    /**
     * @brief 获取日志函数名称集合
     * @return 日志函数名称集合
     */
    const std::unordered_set<std::string>& getLogFunctionNames() const;

    /**
     * @brief 获取指定函数的日志级别
     * @param functionName 函数名
     * @return 日志级别
     */
    LogLevel getLogLevel(const std::string& functionName) const;

    /**
     * @brief 提取日志消息
     * @param node AST节点
     * @return 日志消息
     */
    std::string extractLogMessage(const ast_analyzer::ASTNodeInfo* node) const;

    /**
     * @brief 从文本中提取分类信息
     * @param text 文本内容
     * @param logCallInfo 日志调用信息
     */
    void extractCategoryFromText(const std::string& text, LogCallInfo& logCallInfo);

private:
    const config::Config& config_;                                              ///< 配置对象引用
    const ast_analyzer::ASTAnalyzer& astAnalyzer_;                              ///< AST分析器引用
    std::unordered_map<std::string, std::vector<LogCallInfo>> logCalls_;        ///< 日志调用信息映射表
    std::unordered_set<std::string> logFunctionNames_;                          ///< 日志函数名集合
    std::unordered_map<std::string, LogLevel> functionNameToLevel_;             ///< 函数名到日志级别的映射
    std::unordered_map<std::string, LogType> functionNameToType_;               ///< 函数名到日志类型的映射

    /**
     * @brief 构建日志函数名集合
     */
    void buildLogFunctionNameSet();

    /**
     * @brief 在AST节点中识别日志调用
     * @param node AST节点
     * @param filePath 文件路径
     */
    void identifyLogCallsInNode(const ast_analyzer::ASTNodeInfo* node, const std::string& filePath);

    /**
     * @brief 检查节点是否为日志调用
     * @param node AST节点
     * @param filePath 文件路径
     * @return 如果是日志调用返回true，否则返回false
     */
    bool isLogCall(const ast_analyzer::ASTNodeInfo* node, const std::string& filePath);

    /**
     * @brief 提取日志调用信息
     * @param node AST节点
     * @param filePath 文件路径
     * @return 日志调用信息
     */
    LogCallInfo extractLogCallInfo(const ast_analyzer::ASTNodeInfo* node, const std::string& filePath);

    /**
     * @brief 确定日志级别
     * @param functionName 函数名
     * @return 日志级别
     */
    LogLevel determineLogLevel(const std::string& functionName) const;

    /**
     * @brief 确定日志类型
     * @param functionName 函数名
     * @return 日志类型
     */
    LogType determineLogType(const std::string& functionName) const;

    /**
     * @brief 提取日志分类信息
     * @param node AST节点
     * @return 分类信息
     */
    std::string extractCategoryInfo(const ast_analyzer::ASTNodeInfo* node) const;
};

} // namespace log_identifier
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_LOG_IDENTIFIER_LOG_IDENTIFIER_H 