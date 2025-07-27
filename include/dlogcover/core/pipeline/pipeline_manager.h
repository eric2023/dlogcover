/**
 * @file pipeline_manager.h
 * @brief 流水线管理器 - 协调各个阶段的执行
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_PIPELINE_PIPELINE_MANAGER_H
#define DLOGCOVER_CORE_PIPELINE_PIPELINE_MANAGER_H

#include <dlogcover/core/pipeline/ast_parsing_stage.h>
#include <dlogcover/core/pipeline/function_analysis_stage.h>
#include <dlogcover/config/config.h>
#include <memory>
#include <vector>
#include <future>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

namespace dlogcover {
namespace core {
namespace pipeline {

/**
 * @brief 流水线执行结果
 */
struct PipelineResults {
    // 总体统计
    size_t total_files_processed{0};                      ///< 处理的文件总数
    size_t total_functions_analyzed{0};                   ///< 分析的函数总数
    size_t total_functions_with_logs{0};                  ///< 包含日志的函数数
    double total_processing_time_ms{0.0};                 ///< 总处理时间
    
    // 阶段统计
    std::string ast_parsing_stats;                        ///< AST解析统计
    std::string function_decomposition_stats;             ///< 函数分解统计
    std::string function_analysis_stats;                  ///< 函数分析统计
    
    // 结果数据
    std::vector<std::shared_ptr<FunctionAnalysisResult>> function_results; ///< 函数分析结果
    
    /**
     * @brief 计算总体覆盖率
     * @return 覆盖率百分比
     */
    double getOverallCoverage() const {
        if (total_functions_analyzed == 0) return 0.0;
        return (total_functions_with_logs * 100.0) / total_functions_analyzed;
    }
    
    /**
     * @brief 获取结果摘要
     * @return 结果摘要字符串
     */
    std::string getSummary() const {
        std::ostringstream summary;
        summary << "流水线执行摘要:\n";
        summary << "  处理文件: " << total_files_processed << "\n";
        summary << "  分析函数: " << total_functions_analyzed << "\n";
        summary << "  包含日志: " << total_functions_with_logs << "\n";
        summary << "  总体覆盖率: " << std::fixed << std::setprecision(2) << getOverallCoverage() << "%\n";
        summary << "  处理时间: " << std::fixed << std::setprecision(2) << total_processing_time_ms << "ms";
        return summary.str();
    }
};

/**
 * @brief 流水线配置
 */
struct PipelineConfig {
    // 队列大小配置
    size_t ast_parsing_queue_size{100};                   ///< AST解析队列大小
    size_t function_decomposition_queue_size{500};        ///< 函数分解队列大小
    size_t function_analysis_queue_size{1000};            ///< 函数分析队列大小
    
    // 工作线程配置
    size_t ast_parsing_workers{2};                        ///< AST解析工作线程数
    size_t function_decomposition_workers{1};             ///< 函数分解工作线程数
    size_t function_analysis_workers{4};                  ///< 函数分析工作线程数
    
    // 性能优化配置
    bool enable_caching{true};                            ///< 是否启用缓存
    bool enable_priority_scheduling{true};                ///< 是否启用优先级调度
    bool enable_dynamic_load_balancing{false};            ///< 是否启用动态负载均衡
    
    // 超时配置
    std::chrono::seconds pipeline_timeout{300};           ///< 流水线超时时间
    std::chrono::seconds stage_timeout{60};               ///< 单阶段超时时间
    
    /**
     * @brief 获取总工作线程数
     * @return 总线程数
     */
    size_t getTotalWorkers() const {
        return ast_parsing_workers + function_decomposition_workers + function_analysis_workers;
    }
    
    /**
     * @brief 根据系统配置自动调整
     * @param system_cores 系统核心数
     */
    void autoAdjust(size_t system_cores) {
        if (system_cores > 0) {
            // 保守的线程分配策略
            ast_parsing_workers = std::max(size_t{1}, system_cores / 4);
            function_decomposition_workers = 1; // 通常不是瓶颈
            function_analysis_workers = std::max(size_t{2}, system_cores / 2);
            
            // 队列大小适应线程数
            ast_parsing_queue_size = ast_parsing_workers * 50;
            function_decomposition_queue_size = function_decomposition_workers * 500;
            function_analysis_queue_size = function_analysis_workers * 250;
        }
    }
};

/**
 * @brief 流水线管理器
 * 
 * 负责协调整个分析流水线的执行，包括：
 * - 各阶段的启动和停止
 * - 数据在阶段间的传递
 * - 性能监控和统计
 * - 错误处理和恢复
 */
class PipelineManager {
public:
    /**
     * @brief 构造函数
     * @param config 应用配置
     * @param pipeline_config 流水线配置
     */
    explicit PipelineManager(const config::Config& config, 
                           const PipelineConfig& pipeline_config = PipelineConfig{});
    
    /**
     * @brief 析构函数
     */
    ~PipelineManager();
    
    /**
     * @brief 启动流水线
     * @return 是否成功启动
     */
    bool start();
    
    /**
     * @brief 停止流水线
     */
    void stop();
    
    /**
     * @brief 处理文件列表
     * @param file_paths 文件路径列表
     * @return 处理结果的future
     */
    std::future<PipelineResults> processFiles(const std::vector<std::string>& file_paths);
    
    /**
     * @brief 处理单个文件
     * @param file_path 文件路径
     * @param compile_args 编译参数
     * @return 是否成功提交
     */
    bool processFile(const std::string& file_path, 
                    const std::vector<std::string>& compile_args = {});
    
    /**
     * @brief 等待所有任务完成
     * @param timeout_ms 超时时间（毫秒）
     * @return 是否在超时前完成
     */
    bool waitForCompletion(int timeout_ms = -1);
    
    /**
     * @brief 获取实时统计信息
     * @return 统计信息字符串
     */
    std::string getRealTimeStats() const;
    
    /**
     * @brief 获取当前结果
     * @return 当前的分析结果
     */
    PipelineResults getCurrentResults() const;
    
    /**
     * @brief 检查流水线是否正在运行
     * @return 运行状态
     */
    bool isRunning() const {
        return is_running_.load();
    }
    
    /**
     * @brief 设置结果回调函数
     * @param callback 结果回调
     */
    void setResultCallback(std::function<void(std::shared_ptr<FunctionAnalysisResult>)> callback) {
        result_callback_ = std::move(callback);
    }

private:
    /**
     * @brief 初始化流水线阶段
     */
    void initializeStages();
    
    /**
     * @brief 连接各个阶段
     */
    void connectStages();
    
    /**
     * @brief 监控流水线状态
     */
    void monitorPipeline();
    
    /**
     * @brief 收集最终结果
     * @param result 函数分析结果
     */
    void collectResult(std::shared_ptr<FunctionAnalysisResult> result);
    
    /**
     * @brief 更新统计信息
     */
    void updateStatistics();
    
    /**
     * @brief 处理流水线错误
     * @param error_msg 错误信息
     */
    void handlePipelineError(const std::string& error_msg);
    
    config::Config config_;                               ///< 应用配置
    PipelineConfig pipeline_config_;                     ///< 流水线配置
    
    // 流水线阶段
    std::unique_ptr<ASTParsingStage> ast_parsing_stage_;          ///< AST解析阶段
    std::unique_ptr<FunctionDecompositionStage> decomposition_stage_; ///< 函数分解阶段
    std::unique_ptr<FunctionAnalysisStage> analysis_stage_;       ///< 函数分析阶段
    
    // 状态管理
    std::atomic<bool> is_running_{false};                ///< 运行状态
    std::atomic<bool> stop_requested_{false};            ///< 停止请求
    
    // 结果收集
    mutable std::mutex results_mutex_;                   ///< 结果互斥锁
    std::vector<std::shared_ptr<FunctionAnalysisResult>> collected_results_; ///< 收集的结果
    std::function<void(std::shared_ptr<FunctionAnalysisResult>)> result_callback_; ///< 结果回调
    
    // 监控线程
    std::thread monitor_thread_;                         ///< 监控线程
    
    // 性能统计
    std::chrono::steady_clock::time_point start_time_;   ///< 开始时间
    std::atomic<size_t> files_submitted_{0};             ///< 提交的文件数
    std::atomic<size_t> results_collected_{0};           ///< 收集的结果数
    
    // 错误处理
    mutable std::mutex error_mutex_;                     ///< 错误互斥锁
    std::vector<std::string> error_messages_;            ///< 错误信息列表
};

/**
 * @brief 流水线构建器
 * 
 * 提供便利的流水线配置和构建方法
 */
class PipelineBuilder {
public:
    /**
     * @brief 构造函数
     * @param config 应用配置
     */
    explicit PipelineBuilder(const config::Config& config) : config_(config) {}
    
    /**
     * @brief 设置工作线程数
     * @param ast_workers AST解析线程数
     * @param decomp_workers 函数分解线程数
     * @param analysis_workers 函数分析线程数
     * @return 构建器引用
     */
    PipelineBuilder& setWorkers(size_t ast_workers, size_t decomp_workers, size_t analysis_workers) {
        pipeline_config_.ast_parsing_workers = ast_workers;
        pipeline_config_.function_decomposition_workers = decomp_workers;
        pipeline_config_.function_analysis_workers = analysis_workers;
        return *this;
    }
    
    /**
     * @brief 设置队列大小
     * @param ast_queue AST解析队列大小
     * @param decomp_queue 函数分解队列大小
     * @param analysis_queue 函数分析队列大小
     * @return 构建器引用
     */
    PipelineBuilder& setQueueSizes(size_t ast_queue, size_t decomp_queue, size_t analysis_queue) {
        pipeline_config_.ast_parsing_queue_size = ast_queue;
        pipeline_config_.function_decomposition_queue_size = decomp_queue;
        pipeline_config_.function_analysis_queue_size = analysis_queue;
        return *this;
    }
    
    /**
     * @brief 启用缓存
     * @param enabled 是否启用
     * @return 构建器引用
     */
    PipelineBuilder& enableCaching(bool enabled = true) {
        pipeline_config_.enable_caching = enabled;
        return *this;
    }
    
    /**
     * @brief 启用优先级调度
     * @param enabled 是否启用
     * @return 构建器引用
     */
    PipelineBuilder& enablePriorityScheduling(bool enabled = true) {
        pipeline_config_.enable_priority_scheduling = enabled;
        return *this;
    }
    
    /**
     * @brief 自动调整配置
     * @return 构建器引用
     */
    PipelineBuilder& autoAdjust() {
        pipeline_config_.autoAdjust(std::thread::hardware_concurrency());
        return *this;
    }
    
    /**
     * @brief 构建流水线管理器
     * @return 流水线管理器实例
     */
    std::unique_ptr<PipelineManager> build() {
        return std::make_unique<PipelineManager>(config_, pipeline_config_);
    }

private:
    config::Config config_;                               ///< 应用配置
    PipelineConfig pipeline_config_;                     ///< 流水线配置
};

} // namespace pipeline
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_PIPELINE_PIPELINE_MANAGER_H 