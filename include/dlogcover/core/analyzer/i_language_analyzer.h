/**
 * @file i_language_analyzer.h
 * @brief 语言分析器抽象接口
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_ANALYZER_I_LANGUAGE_ANALYZER_H
#define DLOGCOVER_CORE_ANALYZER_I_LANGUAGE_ANALYZER_H

#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <string>
#include <vector>
#include <memory>

namespace dlogcover {
namespace core {
namespace analyzer {

/**
 * @brief 语言分析器抽象接口
 * 
 * 定义了所有语言分析器必须实现的基本接口，支持多语言扩展
 */
class ILanguageAnalyzer {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~ILanguageAnalyzer() = default;

    /**
     * @brief 分析指定文件
     * @param filePath 要分析的文件路径
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    virtual ast_analyzer::Result<bool> analyze(const std::string& filePath) = 0;

    /**
     * @brief 获取分析结果
     * @return AST节点信息列表的常量引用
     */
    virtual const std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>& getResults() const = 0;

    /**
     * @brief 清空分析结果
     */
    virtual void clear() = 0;

    /**
     * @brief 获取分析器支持的语言名称
     * @return 语言名称字符串
     */
    virtual std::string getLanguageName() const = 0;

    /**
     * @brief 检查分析器是否已启用
     * @return 如果启用返回true，否则返回false
     */
    virtual bool isEnabled() const = 0;

    /**
     * @brief 获取分析器版本信息
     * @return 版本信息字符串
     */
    virtual std::string getVersion() const {
        return "1.0.0";
    }

    /**
     * @brief 获取支持的文件扩展名列表
     * @return 文件扩展名列表
     */
    virtual std::vector<std::string> getSupportedExtensions() const = 0;

    /**
     * @brief 获取分析统计信息
     * @return 统计信息字符串
     */
    virtual std::string getStatistics() const {
        return "No statistics available";
    }

    /**
     * @brief 设置并行模式
     * @param enabled 是否启用并行模式
     * @param maxThreads 最大线程数，0表示自动检测
     * @note 默认实现为空，子类可以选择性重写
     */
    virtual void setParallelMode(bool enabled, size_t maxThreads = 0) {
        // 默认实现为空，子类可以选择性重写
        (void)enabled;
        (void)maxThreads;
    }
};

} // namespace analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_ANALYZER_I_LANGUAGE_ANALYZER_H 