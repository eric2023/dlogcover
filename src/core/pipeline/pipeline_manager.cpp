/**
 * @file pipeline_manager.cpp
 * @brief 流水线管理器实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/pipeline/pipeline_manager.h>
#include <dlogcover/utils/log_utils.h>
#include <algorithm>

namespace dlogcover {
namespace core {
namespace pipeline {

PipelineManager::PipelineManager(const config::Config& config, 
                                const PipelineConfig& pipeline_config)
    : config_(config)
    , pipeline_config_(pipeline_config) {
    
    LOG_INFO_FMT("流水线管理器初始化：总工作线程=%zu", pipeline_config_.getTotalWorkers());
    
    // 初始化阶段
    initializeStages();
    
    LOG_INFO("流水线管理器初始化完成");
}

PipelineManager::~PipelineManager() {
    LOG_INFO("流水线管理器析构开始");
    stop();
    
    // 等待监控线程结束
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    LOG_INFO("流水线管理器析构完成");
}

bool PipelineManager::start() {
    if (is_running_.load()) {
        LOG_WARNING("流水线已在运行中");
        return true;
    }
    
    LOG_INFO("启动流水线系统...");
    start_time_ = std::chrono::steady_clock::now();
    
    try {
        // 连接各个阶段
        connectStages();
        
        // 启动各个阶段
        if (!ast_parsing_stage_->isRunning()) {
            ast_parsing_stage_->start();
            LOG_INFO("AST解析阶段启动完成");
        }
        
        if (!decomposition_stage_->isRunning()) {
            decomposition_stage_->start();
            LOG_INFO("函数分解阶段启动完成");
        }
        
        if (!analysis_stage_->isRunning()) {
            analysis_stage_->start();
            LOG_INFO("函数分析阶段启动完成");
        }
        
        // 启动监控线程
        stop_requested_.store(false);
        monitor_thread_ = std::thread([this] { monitorPipeline(); });
        
        is_running_.store(true);
        
        LOG_INFO("流水线系统启动成功");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("流水线启动失败: %s", e.what());
        stop();
        return false;
    }
}

void PipelineManager::stop() {
    if (!is_running_.load()) {
        return;
    }
    
    LOG_INFO("停止流水线系统...");
    
    stop_requested_.store(true);
    is_running_.store(false);
    
    // 停止各个阶段
    if (analysis_stage_ && analysis_stage_->isRunning()) {
        analysis_stage_->stop();
        LOG_INFO("函数分析阶段停止完成");
    }
    
    if (decomposition_stage_ && decomposition_stage_->isRunning()) {
        decomposition_stage_->stop();
        LOG_INFO("函数分解阶段停止完成");
    }
    
    if (ast_parsing_stage_ && ast_parsing_stage_->isRunning()) {
        ast_parsing_stage_->stop();
        LOG_INFO("AST解析阶段停止完成");
    }
    
    // 等待监控线程结束
    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }
    
    LOG_INFO("流水线系统停止完成");
}

std::future<PipelineResults> PipelineManager::processFiles(const std::vector<std::string>& file_paths) {
    auto promise = std::make_shared<std::promise<PipelineResults>>();
    auto future = promise->get_future();
    
    if (!is_running_.load()) {
        LOG_ERROR("流水线未运行，无法处理文件");
        promise->set_exception(std::make_exception_ptr(
            std::runtime_error("Pipeline is not running")));
        return future;
    }
    
    LOG_INFO_FMT("开始处理 %zu 个文件", file_paths.size());
    
    // 异步处理文件列表
    std::thread([this, file_paths, promise]() {
        try {
            // 提交所有文件
            for (const auto& file_path : file_paths) {
                if (!processFile(file_path)) {
                    LOG_WARNING_FMT("文件提交失败: %s", file_path.c_str());
                }
            }
            
            files_submitted_.store(file_paths.size());
            
            // 等待所有任务完成
            const int timeout_ms = static_cast<int>(pipeline_config_.pipeline_timeout.count() * 1000);
            if (waitForCompletion(timeout_ms)) {
                auto results = getCurrentResults();
                results.total_files_processed = file_paths.size();
                promise->set_value(results);
            } else {
                throw std::runtime_error("Pipeline processing timeout");
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR_FMT("批量处理失败: %s", e.what());
            promise->set_exception(std::current_exception());
        }
    }).detach();
    
    return future;
}

bool PipelineManager::processFile(const std::string& file_path, 
                                 const std::vector<std::string>& compile_args) {
    if (!is_running_.load()) {
        LOG_ERROR("流水线未运行");
        return false;
    }
    
    try {
        // 创建源文件信息
        auto source_info = std::make_shared<SourceFileInfo>(file_path);
        source_info->compile_args = compile_args;
        
        // 创建数据包并提交到AST解析阶段
        auto packet = std::make_shared<PipelinePacket<SourceFileInfo>>(source_info, file_path);
        
        if (ast_parsing_stage_->enqueue(packet)) {
            LOG_DEBUG_FMT("文件已提交: %s", file_path.c_str());
            return true;
        } else {
            LOG_WARNING_FMT("文件提交失败（队列满）: %s", file_path.c_str());
            return false;
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR_FMT("处理文件异常: %s, 错误: %s", file_path.c_str(), e.what());
        return false;
    }
}

bool PipelineManager::waitForCompletion(int timeout_ms) {
    auto start_time = std::chrono::steady_clock::now();
    auto timeout_duration = std::chrono::milliseconds(timeout_ms);
    
    while (is_running_.load() && !stop_requested_.load()) {
        // 检查是否所有任务都已完成
        bool all_stages_idle = true;
        
        if (ast_parsing_stage_ && ast_parsing_stage_->getStats().queue_size.load() > 0) {
            all_stages_idle = false;
        }
        
        if (decomposition_stage_ && decomposition_stage_->getStats().queue_size.load() > 0) {
            all_stages_idle = false;
        }
        
        if (analysis_stage_ && analysis_stage_->getStats().queue_size.load() > 0) {
            all_stages_idle = false;
        }
        
        if (all_stages_idle) {
            LOG_INFO("所有任务已完成");
            return true;
        }
        
        // 检查超时
        if (timeout_ms > 0) {
            auto elapsed = std::chrono::steady_clock::now() - start_time;
            if (elapsed >= timeout_duration) {
                LOG_WARNING_FMT("等待完成超时: %dms", timeout_ms);
                return false;
            }
        }
        
        // 短暂等待
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return false;
}

std::string PipelineManager::getRealTimeStats() const {
    std::ostringstream stats;
    stats << "=== 流水线实时统计 ===\n";
    
    // 总体状态
    stats << "运行状态: " << (is_running_.load() ? "运行中" : "已停止") << "\n";
    stats << "提交文件: " << files_submitted_.load() << "\n";
    stats << "收集结果: " << results_collected_.load() << "\n";
    
    // 各阶段统计
    if (ast_parsing_stage_) {
        stats << "\n[AST解析阶段]\n";
        stats << ast_parsing_stage_->getStats().toString() << "\n";
        stats << ast_parsing_stage_->getParsingStats() << "\n";
    }
    
    if (decomposition_stage_) {
        stats << "\n[函数分解阶段]\n";
        stats << decomposition_stage_->getStats().toString() << "\n";
        stats << decomposition_stage_->getDecompositionStats() << "\n";
    }
    
    if (analysis_stage_) {
        stats << "\n[函数分析阶段]\n";
        stats << analysis_stage_->getStats().toString() << "\n";
        stats << analysis_stage_->getAnalysisStats() << "\n";
    }
    
    // 错误信息
    {
        std::lock_guard<std::mutex> lock(error_mutex_);
        if (!error_messages_.empty()) {
            stats << "\n[错误信息]\n";
            for (const auto& error : error_messages_) {
                stats << "- " << error << "\n";
            }
        }
    }
    
    return stats.str();
}

PipelineResults PipelineManager::getCurrentResults() const {
    PipelineResults results;
    
    std::lock_guard<std::mutex> lock(results_mutex_);
    
    // 复制结果数据
    results.function_results = collected_results_;
    results.total_functions_analyzed = collected_results_.size();
    
    // 统计包含日志的函数
    for (const auto& result : collected_results_) {
        if (result && result->contains_log_calls) {
            results.total_functions_with_logs++;
        }
    }
    
    // 计算总处理时间
    if (is_running_.load()) {
        auto current_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(current_time - start_time_);
        results.total_processing_time_ms = duration.count() / 1000.0;
    }
    
    // 收集各阶段统计
    if (ast_parsing_stage_) {
        results.ast_parsing_stats = ast_parsing_stage_->getParsingStats();
    }
    
    if (decomposition_stage_) {
        results.function_decomposition_stats = decomposition_stage_->getDecompositionStats();
    }
    
    if (analysis_stage_) {
        results.function_analysis_stats = analysis_stage_->getAnalysisStats();
    }
    
    return results;
}

void PipelineManager::initializeStages() {
    LOG_INFO("初始化流水线各阶段...");
    
    // 创建AST解析阶段
    ast_parsing_stage_ = std::make_unique<ASTParsingStage>(
        config_,
        pipeline_config_.ast_parsing_queue_size,
        pipeline_config_.ast_parsing_workers
    );
    ast_parsing_stage_->setCacheEnabled(pipeline_config_.enable_caching);
    
    // 创建函数分解阶段
    decomposition_stage_ = std::make_unique<FunctionDecompositionStage>(
        pipeline_config_.function_decomposition_queue_size,
        pipeline_config_.function_decomposition_workers
    );
    
    // 创建函数分析阶段
    analysis_stage_ = std::make_unique<FunctionAnalysisStage>(
        config_,
        pipeline_config_.function_analysis_queue_size,
        pipeline_config_.function_analysis_workers
    );
    analysis_stage_->setPriorityScheduling(pipeline_config_.enable_priority_scheduling);
    
    LOG_INFO("流水线各阶段初始化完成");
}

void PipelineManager::connectStages() {
    LOG_INFO("连接流水线各阶段...");
    
    // 连接AST解析阶段到函数分解阶段
    ast_parsing_stage_->setNextStageCallback(
        [this](std::shared_ptr<PipelinePacket<ParsedASTInfo>> packet) {
            if (packet && packet->data) {
                if (!decomposition_stage_->enqueue(packet)) {
                    LOG_WARNING_FMT("函数分解阶段队列满，丢弃数据: %s", 
                                   packet->source_file.c_str());
                }
            }
        }
    );
    
    // 连接函数分解阶段到函数分析阶段
    decomposition_stage_->setNextStageCallback(
        [this](std::shared_ptr<PipelinePacket<FunctionTask>> packet) {
            if (packet && packet->data) {
                if (!analysis_stage_->enqueue(packet)) {
                    LOG_WARNING_FMT("函数分析阶段队列满，丢弃任务: %s", 
                                   packet->data->function_name.c_str());
                }
            }
        }
    );
    
    // 连接函数分析阶段到结果收集
    analysis_stage_->setNextStageCallback(
        [this](std::shared_ptr<PipelinePacket<FunctionAnalysisResult>> packet) {
            if (packet && packet->data) {
                collectResult(packet->data);
            }
        }
    );
    
    LOG_INFO("流水线各阶段连接完成");
}

void PipelineManager::monitorPipeline() {
    LOG_INFO("流水线监控线程启动");
    
    while (!stop_requested_.load() && is_running_.load()) {
        try {
            updateStatistics();
            
            // 每隔5秒输出统计信息
            static auto last_stats_time = std::chrono::steady_clock::now();
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - last_stats_time);
            
            if (elapsed.count() >= 5) {
                LOG_INFO_FMT("流水线状态: %s", getRealTimeStats().c_str());
                last_stats_time = current_time;
            }
            
        } catch (const std::exception& e) {
            handlePipelineError("监控线程异常: " + std::string(e.what()));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    
    LOG_INFO("流水线监控线程停止");
}

void PipelineManager::collectResult(std::shared_ptr<FunctionAnalysisResult> result) {
    if (!result) {
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(results_mutex_);
        collected_results_.push_back(result);
    }
    
    results_collected_.fetch_add(1);
    
    // 调用用户回调
    if (result_callback_) {
        try {
            result_callback_(result);
        } catch (const std::exception& e) {
            LOG_WARNING_FMT("结果回调异常: %s", e.what());
        }
    }
    
    LOG_DEBUG_FMT("收集分析结果: %s::%s, 覆盖率: %.2f%%", 
                  result->file_path.c_str(), 
                  result->function_name.c_str(),
                  result->getFunctionCoverage());
}

void PipelineManager::updateStatistics() {
    // 这里可以添加更多的统计更新逻辑
    // 比如性能指标收集、内存使用监控等
    
    // 检查各阶段是否有错误
    if (ast_parsing_stage_) {
        const auto& stats = ast_parsing_stage_->getStats();
        if (stats.packets_dropped.load() > 0) {
            LOG_WARNING_FMT("AST解析阶段有 %zu 个数据包被丢弃", 
                           stats.packets_dropped.load());
        }
    }
    
    if (decomposition_stage_) {
        const auto& stats = decomposition_stage_->getStats();
        if (stats.packets_dropped.load() > 0) {
            LOG_WARNING_FMT("函数分解阶段有 %zu 个数据包被丢弃", 
                           stats.packets_dropped.load());
        }
    }
    
    if (analysis_stage_) {
        const auto& stats = analysis_stage_->getStats();
        if (stats.packets_dropped.load() > 0) {
            LOG_WARNING_FMT("函数分析阶段有 %zu 个数据包被丢弃", 
                           stats.packets_dropped.load());
        }
    }
}

void PipelineManager::handlePipelineError(const std::string& error_msg) {
    LOG_ERROR_FMT("流水线错误: %s", error_msg.c_str());
    
    std::lock_guard<std::mutex> lock(error_mutex_);
    error_messages_.push_back(error_msg);
    
    // 保持错误列表大小在合理范围内
    if (error_messages_.size() > 100) {
        error_messages_.erase(error_messages_.begin());
    }
}

} // namespace pipeline
} // namespace core
} // namespace dlogcover 