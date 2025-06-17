/**
 * @file go_analyzer.h
 * @brief Go语言分析器 - 通过外部工具集成实现Go语言AST分析
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_ANALYZER_GO_ANALYZER_H
#define DLOGCOVER_CORE_ANALYZER_GO_ANALYZER_H

#include <dlogcover/core/analyzer/i_language_analyzer.h>
#include <dlogcover/config/config.h>

#include <memory>
#include <string>

namespace dlogcover {
namespace core {
namespace analyzer {

/**
 * @brief Go语言分析器类
 * 
 * 通过调用外部Go分析器工具实现Go语言的AST分析和日志函数识别
 * 注意：这是阶段1的占位符实现，完整功能将在阶段2实现
 */
class GoAnalyzer : public ILanguageAnalyzer {
public:
    /**
     * @brief 构造函数
     * @param config 配置对象引用
     */
    explicit GoAnalyzer(const config::Config& config);

    /**
     * @brief 析构函数
     */
    ~GoAnalyzer() override;

    /**
     * @brief 分析指定文件
     * @param filePath 要分析的文件路径
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> analyze(const std::string& filePath) override;

    /**
     * @brief 获取分析结果
     * @return AST节点信息列表的常量引用
     */
    const std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>& getResults() const override;

    /**
     * @brief 清空分析结果
     */
    void clear() override;

    /**
     * @brief 获取分析器支持的语言名称
     * @return 语言名称字符串
     */
    std::string getLanguageName() const override;

    /**
     * @brief 检查分析器是否已启用
     * @return 如果启用返回true，否则返回false
     */
    bool isEnabled() const override;

    /**
     * @brief 获取支持的文件扩展名列表
     * @return 文件扩展名列表
     */
    std::vector<std::string> getSupportedExtensions() const override;

    /**
     * @brief 获取分析统计信息
     * @return 统计信息字符串
     */
    std::string getStatistics() const override;

    /**
     * @brief 设置并行模式
     * @param enabled 是否启用并行模式
     * @param maxThreads 最大线程数，0表示自动检测
     */
    void setParallelMode(bool enabled, size_t maxThreads = 0);

    /**
     * @brief 批量分析多个Go文件
     * @param filePaths 要分析的文件路径列表
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> analyzeFiles(const std::vector<std::string>& filePaths);

private:
    /// 配置对象引用
    const config::Config& config_;
    
    /// 分析结果存储
    std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>> results_;
    
    /// Go分析器工具路径
    std::string goAnalyzerPath_;
    
    /// 并行模式配置
    bool parallelEnabled_ = false;
    size_t maxThreads_ = 1;
    
    /// 分析统计信息
    struct Statistics {
        size_t analyzedFiles = 0;
        size_t totalFunctions = 0;
        size_t totalLogCalls = 0;
    } statistics_;

    /**
     * @brief 查找Go分析器工具
     * @return Go分析器工具路径
     */
    std::string findGoAnalyzerTool();

    /**
     * @brief 执行外部Go分析器
     * @param filePath 要分析的文件路径
     * @return 分析结果JSON字符串
     */
    ast_analyzer::Result<std::string> executeGoAnalyzer(const std::string& filePath);

    /**
     * @brief 生成Go分析器配置文件
     * @return 配置文件路径
     */
    std::string generateGoConfig();

    /**
     * @brief 解析Go分析器结果
     * @param jsonResult JSON格式的分析结果
     * @return 解析后的AST节点信息列表
     */
    ast_analyzer::Result<std::vector<std::unique_ptr<ast_analyzer::ASTNodeInfo>>> 
    parseGoAnalysisResult(const std::string& jsonResult);

    /**
     * @brief 执行系统命令
     * @param command 要执行的命令
     * @return 命令输出结果
     */
    ast_analyzer::Result<std::string> executeCommand(const std::string& command);

    /**
     * @brief 串行分析多个Go文件
     * @param filePaths 要分析的文件路径列表
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> analyzeFilesSerial(const std::vector<std::string>& filePaths);

    /**
     * @brief 并行分析多个Go文件
     * @param filePaths 要分析的文件路径列表
     * @return 分析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> analyzeFilesParallel(const std::vector<std::string>& filePaths);

    /**
     * @brief 生成批量分析配置文件
     * @param filePaths 要分析的文件路径列表
     * @param numThreads 使用的线程数量
     * @return 配置文件路径
     */
    std::string generateBatchAnalysisConfig(const std::vector<std::string>& filePaths, size_t numThreads);

    /**
     * @brief 解析批量分析结果
     * @param jsonResult JSON格式的批量分析结果
     * @return 解析结果，成功返回true，失败返回错误信息
     */
    ast_analyzer::Result<bool> parseBatchAnalysisResult(const std::string& jsonResult);
};

} // namespace analyzer
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_ANALYZER_GO_ANALYZER_H 