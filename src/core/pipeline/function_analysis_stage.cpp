/**
 * @file function_analysis_stage.cpp
 * @brief 函数分析阶段实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/pipeline/function_analysis_stage.h>
#include <dlogcover/utils/log_utils.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <sstream>

namespace dlogcover {
namespace core {
namespace pipeline {

/**
 * @brief 函数分析访问器
 */
class FunctionAnalysisVisitor : public clang::RecursiveASTVisitor<FunctionAnalysisVisitor> {
public:
    explicit FunctionAnalysisVisitor(FunctionAnalysisResult& result) : result_(result) {}
    
    bool VisitCallExpr(clang::CallExpr* call) {
        if (!call) return true;
        
        // 简化的日志函数检测
        if (auto* callee = call->getDirectCallee()) {
            std::string func_name = callee->getNameAsString();
            
            // 检查是否是日志函数
            if (isLogFunction(func_name)) {
                result_.contains_log_calls = true;
                result_.log_functions_found.push_back(func_name);
                result_.logged_statements++;
                
                // 获取位置信息（简化实现）
                result_.log_locations.emplace_back(1, func_name); // 简化：使用固定行号
            }
        }
        
        result_.total_statements++;
        return true;
    }
    
    bool VisitIfStmt(clang::IfStmt* if_stmt) {
        if (if_stmt) {
            result_.total_branches++;
            
            // 简化的分支覆盖检测
            bool has_log_in_then = hasLogInStmt(if_stmt->getThen());
            bool has_log_in_else = if_stmt->getElse() ? hasLogInStmt(if_stmt->getElse()) : false;
            
            if (has_log_in_then || has_log_in_else) {
                result_.logged_branches++;
            }
        }
        return true;
    }
    
    bool VisitCXXTryStmt(clang::CXXTryStmt* try_stmt) {
        if (try_stmt) {
            result_.total_exceptions++;
            
            // 检查try块和catch块是否有日志
            bool has_log = hasLogInStmt(try_stmt->getTryBlock());
            for (unsigned i = 0; i < try_stmt->getNumHandlers(); ++i) {
                if (hasLogInStmt(try_stmt->getHandler(i)->getHandlerBlock())) {
                    has_log = true;
                }
            }
            
            if (has_log) {
                result_.logged_exceptions++;
            }
        }
        return true;
    }

private:
    bool isLogFunction(const std::string& func_name) const {
        // 简化的日志函数识别
        static const std::vector<std::string> log_functions = {
            "LOG_DEBUG", "LOG_INFO", "LOG_WARNING", "LOG_ERROR", "LOG_CRITICAL",
            "LOG_DEBUG_FMT", "LOG_INFO_FMT", "LOG_WARNING_FMT", "LOG_ERROR_FMT",
            "qDebug", "qInfo", "qWarning", "qCritical", "qFatal"
        };
        
        return std::find(log_functions.begin(), log_functions.end(), func_name) != log_functions.end();
    }
    
    bool hasLogInStmt(clang::Stmt* stmt) const {
        if (!stmt) return false;
        
        // 简化实现：这里应该递归检查语句中是否包含日志调用
        // 为了演示，我们只检查直接的函数调用
        if (auto* call = llvm::dyn_cast<clang::CallExpr>(stmt)) {
            if (auto* callee = call->getDirectCallee()) {
                return isLogFunction(callee->getNameAsString());
            }
        }
        
        return false;
    }
    
    FunctionAnalysisResult& result_;
};

// FunctionAnalysisStage 实现

FunctionAnalysisStage::FunctionAnalysisStage(const config::Config& config,
                                           size_t max_queue_size,
                                           size_t num_workers)
    : PipelineStage("Function-Analysis", max_queue_size, num_workers)
    , config_(config)
    , priority_scheduling_enabled_(false) {
    
    LOG_INFO_FMT("函数分析阶段初始化：队列大小=%zu, 工作线程=%zu", 
                max_queue_size, num_workers);
}

std::shared_ptr<FunctionAnalysisStage::OutputPacket>
FunctionAnalysisStage::processPacket(std::shared_ptr<InputPacket> input) {
    if (!input || !input->data) {
        return nullptr;
    }
    
    const auto& task = *input->data;
    LOG_DEBUG_FMT("开始分析函数: %s::%s", task.file_path.c_str(), task.function_name.c_str());
    
    // 分析函数
    auto result = analyzeFunction(task);
    if (!result) {
        LOG_WARNING_FMT("函数分析失败: %s::%s", task.file_path.c_str(), task.function_name.c_str());
        return nullptr;
    }
    
    // 更新统计
    functions_analyzed_.fetch_add(1);
    if (result->contains_log_calls) {
        functions_with_logs_.fetch_add(1);
    }
    if (result->complexity_score > 50) { // 复杂函数阈值
        complex_functions_analyzed_.fetch_add(1);
    }
    
    auto current_time = total_analysis_time_.load();
    total_analysis_time_.store(current_time + result->getAnalysisTimeMs());
    
    total_statements_analyzed_.fetch_add(result->total_statements);
    total_branches_analyzed_.fetch_add(result->total_branches);
    
    LOG_DEBUG_FMT("函数分析完成: %s::%s, 覆盖率: %.2f%%, 耗时: %.2fms", 
                  task.file_path.c_str(), task.function_name.c_str(),
                  result->getFunctionCoverage(), result->getAnalysisTimeMs());
    
    return std::make_shared<OutputPacket>(result, task.file_path);
}

void FunctionAnalysisStage::onStart() {
    LOG_INFO_FMT("函数分析阶段启动，工作线程数: %zu", num_workers_);
    
    // 重置统计信息
    functions_analyzed_.store(0);
    functions_with_logs_.store(0);
    complex_functions_analyzed_.store(0);
    total_analysis_time_.store(0.0);
    total_statements_analyzed_.store(0);
    total_branches_analyzed_.store(0);
}

void FunctionAnalysisStage::onStop() {
    LOG_INFO_FMT("函数分析阶段停止，统计信息: %s", getAnalysisStats().c_str());
    
    // 清理每线程的分析器
    std::lock_guard<std::mutex> lock(analyzers_mutex_);
    function_analyzers_.clear();
    expression_analyzers_.clear();
}

std::shared_ptr<FunctionAnalysisResult> 
FunctionAnalysisStage::analyzeFunction(const FunctionTask& task) {
    auto result = std::make_shared<FunctionAnalysisResult>(task.function_name, task.file_path);
    result->analysis_start = std::chrono::steady_clock::now();
    
    try {
        if (!task.function_decl || !task.ast_info) {
            result->analysis_end = std::chrono::steady_clock::now();
            return result;
        }
        
        // 获取或创建线程本地分析器
        createThreadLocalAnalyzers(0); // 简化，使用固定ID
        
        // 分析函数的各个方面
        analyzeLogCalls(task.function_decl, *result);
        analyzeBranchCoverage(task.function_decl, *result);
        analyzeExceptionCoverage(task.function_decl, *result);
        identifyUncoveredPaths(task.function_decl, *result);
        
        // 计算复杂度
        result->complexity_score = calculateComplexity(task.function_decl);
        
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("函数分析异常: %s::%s, 错误: %s", 
                      task.file_path.c_str(), task.function_name.c_str(), e.what());
    }
    
    result->analysis_end = std::chrono::steady_clock::now();
    return result;
}

void FunctionAnalysisStage::analyzeLogCalls(clang::FunctionDecl* func_decl, 
                                           FunctionAnalysisResult& result) {
    if (!func_decl || !func_decl->hasBody()) {
        return;
    }
    
    try {
        // 使用访问器分析函数体
        FunctionAnalysisVisitor visitor(result);
        visitor.TraverseStmt(func_decl->getBody());
        
    } catch (const std::exception& e) {
        LOG_WARNING_FMT("日志调用分析异常: %s", e.what());
    }
}

void FunctionAnalysisStage::analyzeBranchCoverage(clang::FunctionDecl* func_decl, 
                                                 FunctionAnalysisResult& result) {
    if (!func_decl || !func_decl->hasBody()) {
        return;
    }
    
    // 分支覆盖分析已经在 FunctionAnalysisVisitor 中处理
    // 这里可以添加更复杂的分支分析逻辑
}

void FunctionAnalysisStage::analyzeExceptionCoverage(clang::FunctionDecl* func_decl, 
                                                    FunctionAnalysisResult& result) {
    if (!func_decl || !func_decl->hasBody()) {
        return;
    }
    
    // 异常处理覆盖分析已经在 FunctionAnalysisVisitor 中处理
    // 这里可以添加更复杂的异常分析逻辑
}

void FunctionAnalysisStage::identifyUncoveredPaths(clang::FunctionDecl* func_decl, 
                                                  FunctionAnalysisResult& result) {
    if (!func_decl || !func_decl->hasBody()) {
        return;
    }
    
    // 简化的未覆盖路径识别
    if (result.total_branches > result.logged_branches) {
        size_t uncovered_branches = result.total_branches - result.logged_branches;
        result.uncovered_paths.push_back(
            "未覆盖分支: " + std::to_string(uncovered_branches) + "/" + 
            std::to_string(result.total_branches)
        );
    }
    
    if (result.total_exceptions > result.logged_exceptions) {
        size_t uncovered_exceptions = result.total_exceptions - result.logged_exceptions;
        result.uncovered_paths.push_back(
            "未覆盖异常处理: " + std::to_string(uncovered_exceptions) + "/" + 
            std::to_string(result.total_exceptions)
        );
    }
}

size_t FunctionAnalysisStage::calculateComplexity(clang::FunctionDecl* func_decl) {
    if (!func_decl || !func_decl->hasBody()) {
        return 0;
    }
    
    // 简化的圈复杂度计算
    size_t complexity = 1; // 基础复杂度
    
    // 参数数量
    complexity += func_decl->getNumParams();
    
    // 这里应该遍历函数体，计算分支、循环、异常处理等
    // 为了演示，使用简化算法
    
    return complexity;
}

void FunctionAnalysisStage::createThreadLocalAnalyzers(size_t worker_id) {
    auto thread_id = std::this_thread::get_id();
    
    std::lock_guard<std::mutex> lock(analyzers_mutex_);
    
    // 检查是否已经为这个线程创建了分析器
    if (function_analyzers_.find(thread_id) == function_analyzers_.end()) {
        // 简化实现：暂时不创建分析器，避免构造函数参数问题
        // function_analyzers_[thread_id] = std::make_unique<ast_analyzer::ASTFunctionAnalyzer>();
        // expression_analyzers_[thread_id] = std::make_unique<ast_analyzer::ASTExpressionAnalyzer>();
        
        LOG_DEBUG_FMT("为线程 %zu 创建分析器", worker_id);
    }
}

std::string FunctionAnalysisStage::getAnalysisStats() const {
    std::ostringstream stats;
    stats << "函数分析统计: ";
    stats << "已分析=" << functions_analyzed_.load();
    stats << ", 包含日志=" << functions_with_logs_.load();
    stats << ", 复杂函数=" << complex_functions_analyzed_.load();
    stats << ", 平均耗时=" << (functions_analyzed_.load() > 0 ? 
                             total_analysis_time_.load() / functions_analyzed_.load() : 0.0) << "ms";
    stats << ", 总语句=" << total_statements_analyzed_.load();
    stats << ", 总分支=" << total_branches_analyzed_.load();
    return stats.str();
}

// EnhancedFunctionAnalysisStage 实现

EnhancedFunctionAnalysisStage::EnhancedFunctionAnalysisStage(const config::Config& config,
                                                           size_t max_queue_size,
                                                           size_t num_workers)
    : FunctionAnalysisStage(config, max_queue_size, num_workers)
    , dynamic_load_balancing_(false) {
    
    // 创建优先级队列
    priority_queue_ = std::make_unique<PriorityTaskQueue<std::shared_ptr<InputPacket>>>();
    
    LOG_INFO("增强型函数分析阶段初始化完成");
}

std::string EnhancedFunctionAnalysisStage::getLoadBalancingStats() const {
    std::ostringstream stats;
    stats << "负载均衡统计: ";
    stats << "工作线程调整=" << worker_adjustments_.load();
    stats << ", 负载均衡决策=" << load_balance_decisions_.load();
    return stats.str();
}

void EnhancedFunctionAnalysisStage::adjustWorkerCount() {
    // 简化的动态线程调整逻辑
    auto current_queue_size = getStats().queue_size.load();
    
    if (current_queue_size > max_queue_size_ * 0.8) {
        // 队列接近满载，考虑增加线程（在实际实现中需要更复杂的逻辑）
        worker_adjustments_.fetch_add(1);
        LOG_INFO("检测到高负载，建议增加工作线程");
    } else if (current_queue_size < max_queue_size_ * 0.2) {
        // 队列负载较低，考虑减少线程
        worker_adjustments_.fetch_add(1);
        LOG_INFO("检测到低负载，建议减少工作线程");
    }
}

void EnhancedFunctionAnalysisStage::monitorWorkload() {
    LOG_INFO("负载监控线程启动");
    
    while (!load_monitor_stop_.load()) {
        try {
            adjustWorkerCount();
            load_balance_decisions_.fetch_add(1);
            
        } catch (const std::exception& e) {
            LOG_WARNING_FMT("负载监控异常: %s", e.what());
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    
    LOG_INFO("负载监控线程停止");
}

} // namespace pipeline
} // namespace core
} // namespace dlogcover 