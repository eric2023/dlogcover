/**
 * @file function_analysis_stage.h
 * @brief 函数分析阶段 - 流水线第二阶段，实现细粒度并行分析
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_PIPELINE_FUNCTION_ANALYSIS_STAGE_H
#define DLOGCOVER_CORE_PIPELINE_FUNCTION_ANALYSIS_STAGE_H

#include <dlogcover/core/pipeline/pipeline_stage.h>
#include <dlogcover/core/pipeline/ast_parsing_stage.h>
#include <dlogcover/core/ast_analyzer/ast_function_analyzer.h>
#include <dlogcover/core/ast_analyzer/ast_expression_analyzer.h>
#include <dlogcover/core/coverage/coverage_stats.h>
#include <dlogcover/config/config.h>
#include <queue>
#include <unordered_map>
#include <thread>

namespace dlogcover {
namespace core {
namespace pipeline {

/**
 * @brief 函数分析结果
 */
struct FunctionAnalysisResult {
    std::string function_name;                            ///< 函数名称
    std::string file_path;                                ///< 所属文件
    
    // 覆盖率信息
    bool contains_log_calls;                              ///< 是否包含日志调用
    size_t total_statements;                              ///< 总语句数
    size_t logged_statements;                             ///< 包含日志的语句数
    size_t total_branches;                                ///< 总分支数
    size_t logged_branches;                               ///< 包含日志的分支数
    size_t total_exceptions;                              ///< 总异常处理数
    size_t logged_exceptions;                             ///< 包含日志的异常处理数
    
    // 日志调用详情
    std::vector<std::string> log_functions_found;         ///< 发现的日志函数
    std::vector<std::pair<int, std::string>> log_locations; ///< 日志位置（行号，内容）
    
    // 未覆盖路径
    std::vector<std::string> uncovered_paths;             ///< 未覆盖的代码路径
    
    // 性能指标
    std::chrono::steady_clock::time_point analysis_start; ///< 分析开始时间
    std::chrono::steady_clock::time_point analysis_end;   ///< 分析结束时间
    size_t complexity_score;                              ///< 复杂度分数
    
    FunctionAnalysisResult(const std::string& name, const std::string& path)
        : function_name(name), file_path(path), contains_log_calls(false),
          total_statements(0), logged_statements(0), total_branches(0), 
          logged_branches(0), total_exceptions(0), logged_exceptions(0),
          complexity_score(0) {}
    
    /**
     * @brief 计算函数覆盖率
     * @return 覆盖率百分比
     */
    double getFunctionCoverage() const {
        if (total_statements == 0) return 0.0;
        return (logged_statements * 100.0) / total_statements;
    }
    
    /**
     * @brief 计算分支覆盖率
     * @return 分支覆盖率百分比
     */
    double getBranchCoverage() const {
        if (total_branches == 0) return 0.0;
        return (logged_branches * 100.0) / total_branches;
    }
    
    /**
     * @brief 获取分析时间
     * @return 分析时间（毫秒）
     */
    double getAnalysisTimeMs() const {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(analysis_end - analysis_start);
        return duration.count() / 1000.0;
    }
};

/**
 * @brief 函数分析阶段
 * 
 * 流水线的第二阶段，负责：
 * - 细粒度的函数级并行分析
 * - 日志覆盖率计算
 * - 未覆盖路径识别
 * - 优化的负载均衡
 */
class FunctionAnalysisStage : public PipelineStage<FunctionTask, FunctionAnalysisResult> {
public:
    /**
     * @brief 构造函数
     * @param config 配置信息
     * @param max_queue_size 最大队列大小
     * @param num_workers 工作线程数量
     */
    explicit FunctionAnalysisStage(const config::Config& config,
                                 size_t max_queue_size = 1000,
                                 size_t num_workers = 4);
    
    /**
     * @brief 析构函数
     */
    ~FunctionAnalysisStage() override = default;
    
    /**
     * @brief 设置优先级调度
     * @param enabled 是否启用优先级调度
     */
    void setPriorityScheduling(bool enabled) {
        priority_scheduling_enabled_ = enabled;
    }
    
    /**
     * @brief 获取分析统计信息
     * @return 统计信息字符串
     */
    std::string getAnalysisStats() const;

protected:
    /**
     * @brief 处理单个函数任务
     * @param input 输入的函数任务
     * @return 函数分析结果
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
     * @brief 分析单个函数
     * @param task 函数任务
     * @return 分析结果
     */
    std::shared_ptr<FunctionAnalysisResult> analyzeFunction(const FunctionTask& task);
    
    /**
     * @brief 分析函数的日志调用
     * @param func_decl 函数声明
     * @param result 分析结果（输出参数）
     */
    void analyzeLogCalls(clang::FunctionDecl* func_decl, FunctionAnalysisResult& result);
    
    /**
     * @brief 分析函数的分支覆盖
     * @param func_decl 函数声明
     * @param result 分析结果（输出参数）
     */
    void analyzeBranchCoverage(clang::FunctionDecl* func_decl, FunctionAnalysisResult& result);
    
    /**
     * @brief 分析异常处理覆盖
     * @param func_decl 函数声明
     * @param result 分析结果（输出参数）
     */
    void analyzeExceptionCoverage(clang::FunctionDecl* func_decl, FunctionAnalysisResult& result);
    
    /**
     * @brief 识别未覆盖路径
     * @param func_decl 函数声明
     * @param result 分析结果（输出参数）
     */
    void identifyUncoveredPaths(clang::FunctionDecl* func_decl, FunctionAnalysisResult& result);
    
    /**
     * @brief 计算函数复杂度
     * @param func_decl 函数声明
     * @return 复杂度分数
     */
    size_t calculateComplexity(clang::FunctionDecl* func_decl);
    
    /**
     * @brief 创建每线程的分析器
     * @param worker_id 工作线程ID
     */
    void createThreadLocalAnalyzers(size_t worker_id);
    
    config::Config config_;                               ///< 配置信息
    bool priority_scheduling_enabled_;                    ///< 是否启用优先级调度
    
    // 每线程的分析器（避免锁竞争）
    mutable std::mutex analyzers_mutex_;                  ///< 分析器互斥锁
    std::unordered_map<std::thread::id, std::unique_ptr<ast_analyzer::ASTFunctionAnalyzer>> function_analyzers_;
    std::unordered_map<std::thread::id, std::unique_ptr<ast_analyzer::ASTExpressionAnalyzer>> expression_analyzers_;
    
    // 性能统计
    std::atomic<size_t> functions_analyzed_{0};           ///< 已分析函数数
    std::atomic<size_t> functions_with_logs_{0};          ///< 包含日志的函数数
    std::atomic<size_t> complex_functions_analyzed_{0};   ///< 分析的复杂函数数
    std::atomic<double> total_analysis_time_{0.0};        ///< 总分析时间
    std::atomic<size_t> total_statements_analyzed_{0};    ///< 总分析语句数
    std::atomic<size_t> total_branches_analyzed_{0};      ///< 总分析分支数
};

/**
 * @brief 优先级任务队列
 * 
 * 支持基于优先级的任务调度，复杂度高的函数优先处理
 */
template<typename T>
class PriorityTaskQueue {
public:
    /**
     * @brief 插入任务
     * @param task 任务
     * @param priority 优先级
     */
    void push(T task, int priority) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.emplace(priority, std::move(task));
        condition_.notify_one();
    }
    
    /**
     * @brief 取出最高优先级任务
     * @param task 输出任务
     * @param timeout_ms 超时时间（毫秒）
     * @return 是否成功取出任务
     */
    bool pop(T& task, int timeout_ms = 100) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (condition_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                               [this] { return !queue_.empty() || stop_; })) {
            if (!queue_.empty()) {
                task = std::move(queue_.top().second);
                queue_.pop();
                return true;
            }
        }
        return false;
    }
    
    /**
     * @brief 停止队列
     */
    void stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
        condition_.notify_all();
    }
    
    /**
     * @brief 获取队列大小
     * @return 队列大小
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    /**
     * @brief 检查队列是否为空
     * @return 是否为空
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    using PriorityTask = std::pair<int, T>;               ///< 优先级任务对
    
    mutable std::mutex mutex_;                            ///< 互斥锁
    std::condition_variable condition_;                   ///< 条件变量
    std::priority_queue<PriorityTask> queue_;             ///< 优先级队列
    bool stop_{false};                                    ///< 停止标志
};

/**
 * @brief 增强型函数分析阶段
 * 
 * 支持优先级调度和动态负载均衡的函数分析
 */
class EnhancedFunctionAnalysisStage : public FunctionAnalysisStage {
public:
    /**
     * @brief 构造函数
     * @param config 配置信息
     * @param max_queue_size 最大队列大小
     * @param num_workers 工作线程数量
     */
    explicit EnhancedFunctionAnalysisStage(const config::Config& config,
                                         size_t max_queue_size = 1000,
                                         size_t num_workers = 4);
    
    /**
     * @brief 启用动态负载均衡
     * @param enabled 是否启用
     */
    void setDynamicLoadBalancing(bool enabled) {
        dynamic_load_balancing_ = enabled;
    }
    
    /**
     * @brief 获取负载均衡统计
     * @return 统计信息
     */
    std::string getLoadBalancingStats() const;

private:
    /**
     * @brief 动态调整工作线程数
     */
    void adjustWorkerCount();
    
    /**
     * @brief 监控工作负载
     */
    void monitorWorkload();
    
    bool dynamic_load_balancing_;                         ///< 是否启用动态负载均衡
    std::unique_ptr<PriorityTaskQueue<std::shared_ptr<InputPacket>>> priority_queue_; ///< 优先级队列
    std::thread load_monitor_thread_;                     ///< 负载监控线程
    std::atomic<bool> load_monitor_stop_{false};          ///< 负载监控停止标志
    
    // 负载均衡统计
    std::atomic<size_t> worker_adjustments_{0};           ///< 工作线程调整次数
    std::atomic<size_t> load_balance_decisions_{0};       ///< 负载均衡决策次数
};

} // namespace pipeline
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_PIPELINE_FUNCTION_ANALYSIS_STAGE_H 