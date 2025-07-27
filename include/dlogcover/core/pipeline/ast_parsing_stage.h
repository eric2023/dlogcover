/**
 * @file ast_parsing_stage.h
 * @brief AST解析阶段 - 流水线第一阶段，负责解析源文件
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_PIPELINE_AST_PARSING_STAGE_H
#define DLOGCOVER_CORE_PIPELINE_AST_PARSING_STAGE_H

#include <dlogcover/core/pipeline/pipeline_stage.h>
#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <dlogcover/config/config.h>
#include <string>
#include <memory>
#include <unordered_map>

// Clang headers
#include <clang/Tooling/Tooling.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/Frontend/CompilerInstance.h>

namespace dlogcover {
namespace core {
namespace pipeline {

/**
 * @brief 源文件信息
 */
struct SourceFileInfo {
    std::string file_path;                                ///< 文件路径
    std::string content;                                  ///< 文件内容（可选，用于内存中的内容）
    std::vector<std::string> compile_args;               ///< 编译参数
    size_t estimated_complexity;                         ///< 预估复杂度（行数等）
    
    SourceFileInfo(const std::string& path) 
        : file_path(path), estimated_complexity(0) {}
};

/**
 * @brief AST解析结果
 */
struct ParsedASTInfo {
    std::string file_path;                                ///< 文件路径
    std::unique_ptr<clang::ASTUnit> ast_unit;             ///< AST单元
    std::unique_ptr<ast_analyzer::ASTNodeInfo> root_node; ///< 根节点信息
    std::vector<std::string> dependencies;               ///< 文件依赖
    bool parse_success;                                   ///< 解析是否成功
    std::string error_message;                            ///< 错误信息
    std::chrono::steady_clock::time_point parse_start;   ///< 解析开始时间
    std::chrono::steady_clock::time_point parse_end;     ///< 解析结束时间
    
    ParsedASTInfo(const std::string& path) 
        : file_path(path), parse_success(false) {}
    
    double getParsingTimeMs() const {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(parse_end - parse_start);
        return duration.count() / 1000.0;
    }
};

/**
 * @brief AST解析阶段
 * 
 * 流水线的第一阶段，负责：
 * - 解析源文件生成AST
 * - 提取基本的AST节点信息
 * - 检测文件依赖关系
 * - 缓存解析结果
 */
class ASTParsingStage : public PipelineStage<SourceFileInfo, ParsedASTInfo> {
public:
    /**
     * @brief 构造函数
     * @param config 配置信息
     * @param max_queue_size 最大队列大小
     * @param num_workers 工作线程数量
     */
    explicit ASTParsingStage(const config::Config& config,
                           size_t max_queue_size = 100,
                           size_t num_workers = 2);
    
    /**
     * @brief 析构函数
     */
    ~ASTParsingStage() override = default;
    
    /**
     * @brief 设置缓存启用状态
     * @param enabled 是否启用缓存
     */
    void setCacheEnabled(bool enabled) {
        cache_enabled_ = enabled;
    }
    
    /**
     * @brief 获取解析统计信息
     * @return 统计信息字符串
     */
    std::string getParsingStats() const;

protected:
    /**
     * @brief 处理单个源文件
     * @param input 输入的源文件信息
     * @return 解析后的AST信息
     */
    std::shared_ptr<OutputPacket> processPacket(std::shared_ptr<InputPacket> input) override;
    
    /**
     * @brief 阶段启动回调
     */
    void onStart() override;
    
    /**
     * @brief 阶段停止回调
     */
    void onStop() override;

private:
    /**
     * @brief 解析单个文件
     * @param source_info 源文件信息
     * @return 解析结果
     */
    std::shared_ptr<ParsedASTInfo> parseSourceFile(const SourceFileInfo& source_info);
    
    /**
     * @brief 从缓存获取AST
     * @param file_path 文件路径
     * @return 缓存的AST信息，nullptr表示未找到
     */
    std::shared_ptr<ParsedASTInfo> getFromCache(const std::string& file_path);
    
    /**
     * @brief 缓存AST结果
     * @param ast_info AST信息
     */
    void cacheASTResult(std::shared_ptr<ParsedASTInfo> ast_info);
    
    /**
     * @brief 扫描文件依赖关系
     * @param ast_context AST上下文
     * @return 依赖文件列表
     */
    std::vector<std::string> scanDependencies(clang::ASTContext& ast_context);
    
    /**
     * @brief 构建编译参数
     * @param source_info 源文件信息
     * @return 编译参数列表
     */
    std::vector<std::string> buildCompileArgs(const SourceFileInfo& source_info);
    
    /**
     * @brief 预估文件复杂度
     * @param file_path 文件路径
     * @return 复杂度分数
     */
    size_t estimateFileComplexity(const std::string& file_path);
    
    config::Config config_;                               ///< 配置信息
    bool cache_enabled_;                                  ///< 是否启用缓存
    
    // 性能统计
    mutable std::mutex stats_mutex_;                      ///< 统计信息互斥锁
    std::atomic<size_t> files_parsed_{0};                 ///< 已解析文件数
    std::atomic<size_t> files_cached_{0};                 ///< 缓存命中数
    std::atomic<size_t> parse_errors_{0};                 ///< 解析错误数
    std::atomic<double> total_parse_time_{0.0};           ///< 总解析时间
    
    // 简单的内存缓存（生产环境中应该使用更复杂的缓存策略）
    mutable std::mutex cache_mutex_;                      ///< 缓存互斥锁
    std::unordered_map<std::string, std::shared_ptr<ParsedASTInfo>> ast_cache_; ///< AST缓存
    static constexpr size_t MAX_CACHE_SIZE = 50;          ///< 最大缓存大小
};

/**
 * @brief 函数任务信息
 * 
 * 用于细粒度并行处理的函数级任务
 */
struct FunctionTask {
    std::string function_name;                            ///< 函数名称
    std::string file_path;                                ///< 所属文件
    clang::FunctionDecl* function_decl;                   ///< 函数声明
    std::shared_ptr<ParsedASTInfo> ast_info;              ///< 所属AST信息
    size_t estimated_complexity;                          ///< 预估复杂度
    int priority;                                         ///< 任务优先级（越高越优先）
    
    FunctionTask(const std::string& name, const std::string& path, 
                clang::FunctionDecl* decl, std::shared_ptr<ParsedASTInfo> ast)
        : function_name(name), file_path(path), function_decl(decl), 
          ast_info(ast), estimated_complexity(0), priority(0) {}
};

/**
 * @brief 函数分解阶段
 * 
 * 将解析好的AST文件分解为独立的函数分析任务，实现细粒度并行
 */
class FunctionDecompositionStage : public PipelineStage<ParsedASTInfo, FunctionTask> {
public:
    /**
     * @brief 构造函数
     * @param max_queue_size 最大队列大小
     * @param num_workers 工作线程数量
     */
    explicit FunctionDecompositionStage(size_t max_queue_size = 500,
                                      size_t num_workers = 1);
    
    /**
     * @brief 获取分解统计信息
     * @return 统计信息字符串
     */
    std::string getDecompositionStats() const;

protected:
    /**
     * @brief 处理AST信息，分解为函数任务
     * @param input 输入的AST信息
     * @return 函数任务（此阶段会产生多个输出）
     */
    std::shared_ptr<OutputPacket> processPacket(std::shared_ptr<InputPacket> input) override;

private:
    /**
     * @brief 提取文件中的所有函数
     * @param ast_info AST信息
     * @return 函数声明列表
     */
    std::vector<clang::FunctionDecl*> extractFunctions(const ParsedASTInfo& ast_info);
    
    /**
     * @brief 计算函数复杂度
     * @param func_decl 函数声明
     * @return 复杂度分数
     */
    size_t calculateFunctionComplexity(clang::FunctionDecl* func_decl);
    
    /**
     * @brief 确定函数优先级
     * @param func_decl 函数声明
     * @param complexity 复杂度
     * @return 优先级分数
     */
    int determineFunctionPriority(clang::FunctionDecl* func_decl, size_t complexity);
    
    // 统计信息
    std::atomic<size_t> files_processed_{0};              ///< 处理的文件数
    std::atomic<size_t> functions_extracted_{0};          ///< 提取的函数数
    std::atomic<size_t> complex_functions_{0};            ///< 复杂函数数（复杂度>阈值）
};

} // namespace pipeline
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_PIPELINE_AST_PARSING_STAGE_H 