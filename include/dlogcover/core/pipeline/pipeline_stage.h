/**
 * @file pipeline_stage.h
 * @brief 流水线阶段抽象基类 - 支持高效的流水线并行处理
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CORE_PIPELINE_PIPELINE_STAGE_H
#define DLOGCOVER_CORE_PIPELINE_PIPELINE_STAGE_H

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <thread>
#include <vector>
#include <chrono>

namespace dlogcover {
namespace core {
namespace pipeline {

/**
 * @brief 流水线数据包
 * 
 * 在流水线各阶段间传递的数据容器
 */
template<typename T>
struct PipelinePacket {
    std::shared_ptr<T> data;                              ///< 数据载荷
    std::chrono::steady_clock::time_point timestamp;     ///< 时间戳
    size_t stage_id;                                      ///< 当前阶段ID
    std::string source_file;                              ///< 源文件路径
    
    PipelinePacket(std::shared_ptr<T> d, const std::string& file) 
        : data(std::move(d)), timestamp(std::chrono::steady_clock::now()), 
          stage_id(0), source_file(file) {}
};

/**
 * @brief 流水线性能统计
 */
struct PipelineStats {
    std::atomic<size_t> packets_processed{0};             ///< 处理的数据包数量
    std::atomic<size_t> packets_dropped{0};               ///< 丢弃的数据包数量
    std::atomic<double> avg_processing_time{0.0};         ///< 平均处理时间(ms)
    std::atomic<size_t> queue_size{0};                    ///< 当前队列大小
    std::atomic<size_t> max_queue_size{0};                ///< 最大队列大小
    
    void reset() {
        packets_processed.store(0);
        packets_dropped.store(0);
        avg_processing_time.store(0.0);
        queue_size.store(0);
        max_queue_size.store(0);
    }
    
    std::string toString() const {
        return "Pipeline Stats: processed=" + std::to_string(packets_processed.load()) +
               ", dropped=" + std::to_string(packets_dropped.load()) +
               ", avg_time=" + std::to_string(avg_processing_time.load()) + "ms" +
               ", queue=" + std::to_string(queue_size.load()) + 
               "/" + std::to_string(max_queue_size.load());
    }
};

/**
 * @brief 流水线阶段抽象基类
 * 
 * 定义流水线处理阶段的标准接口，支持：
 * - 异步数据处理
 * - 背压控制
 * - 性能监控
 * - 优雅关闭
 */
template<typename InputType, typename OutputType>
class PipelineStage {
public:
    using InputPacket = PipelinePacket<InputType>;
    using OutputPacket = PipelinePacket<OutputType>;
    using NextStageCallback = std::function<void(std::shared_ptr<OutputPacket>)>;
    
    /**
     * @brief 构造函数
     * @param stage_name 阶段名称
     * @param max_queue_size 最大队列大小（背压控制）
     * @param num_workers 工作线程数量
     */
    explicit PipelineStage(const std::string& stage_name, 
                          size_t max_queue_size = 1000,
                          size_t num_workers = 1)
        : stage_name_(stage_name)
        , max_queue_size_(max_queue_size)
        , num_workers_(num_workers)
        , stop_requested_(false)
        , is_running_(false) {}
    
    /**
     * @brief 虚析构函数
     */
    virtual ~PipelineStage() {
        stop();
    }
    
    /**
     * @brief 启动流水线阶段
     */
    virtual void start() {
        if (is_running_.load()) {
            return;
        }
        
        stop_requested_.store(false);
        is_running_.store(true);
        
        // 启动工作线程
        workers_.reserve(num_workers_);
        for (size_t i = 0; i < num_workers_; ++i) {
            workers_.emplace_back([this, i] {
                workerThread(i);
            });
        }
        
        onStart();
    }
    
    /**
     * @brief 停止流水线阶段
     */
    virtual void stop() {
        if (!is_running_.load()) {
            return;
        }
        
        stop_requested_.store(true);
        
        // 通知所有等待的线程
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            input_condition_.notify_all();
        }
        
        // 等待所有工作线程完成
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        
        workers_.clear();
        is_running_.store(false);
        
        onStop();
    }
    
    /**
     * @brief 向阶段输入数据
     * @param packet 输入数据包
     * @return 是否成功入队（false表示队列满或已停止）
     */
    bool enqueue(std::shared_ptr<InputPacket> packet) {
        if (stop_requested_.load()) {
            stats_.packets_dropped.fetch_add(1);
            return false;
        }
        
        std::unique_lock<std::mutex> lock(queue_mutex_);
        
        // 背压控制：如果队列满了，等待或丢弃
        if (input_queue_.size() >= max_queue_size_) {
            if (input_condition_.wait_for(lock, std::chrono::milliseconds(100),
                [this] { return input_queue_.size() < max_queue_size_ || stop_requested_.load(); })) {
                if (stop_requested_.load()) {
                    stats_.packets_dropped.fetch_add(1);
                    return false;
                }
            } else {
                // 超时，丢弃数据包
                stats_.packets_dropped.fetch_add(1);
                return false;
            }
        }
        
        input_queue_.push(packet);
        stats_.queue_size.store(input_queue_.size());
        stats_.max_queue_size.store(std::max(stats_.max_queue_size.load(), input_queue_.size()));
        
        input_condition_.notify_one();
        return true;
    }
    
    /**
     * @brief 设置下一阶段的回调函数
     * @param callback 回调函数
     */
    void setNextStageCallback(NextStageCallback callback) {
        next_stage_callback_ = std::move(callback);
    }
    
    /**
     * @brief 获取性能统计信息
     * @return 性能统计对象
     */
    const PipelineStats& getStats() const {
        return stats_;
    }
    
    /**
     * @brief 获取阶段名称
     * @return 阶段名称
     */
    const std::string& getStageName() const {
        return stage_name_;
    }
    
    /**
     * @brief 检查阶段是否正在运行
     * @return 运行状态
     */
    bool isRunning() const {
        return is_running_.load();
    }

protected:
    /**
     * @brief 处理单个数据包（子类必须实现）
     * @param input 输入数据包
     * @return 输出数据包，nullptr表示过滤掉
     */
    virtual std::shared_ptr<OutputPacket> processPacket(std::shared_ptr<InputPacket> input) = 0;
    
    /**
     * @brief 阶段启动时的回调（子类可选实现）
     */
    virtual void onStart() {}
    
    /**
     * @brief 阶段停止时的回调（子类可选实现）
     */
    virtual void onStop() {}
    
    /**
     * @brief 发送数据到下一阶段
     * @param output 输出数据包
     */
    void sendToNextStage(std::shared_ptr<OutputPacket> output) {
        if (next_stage_callback_ && output) {
            output->stage_id++;
            next_stage_callback_(output);
        }
    }

private:
    /**
     * @brief 工作线程函数
     * @param worker_id 工作线程ID
     */
    void workerThread(size_t worker_id) {
        while (!stop_requested_.load()) {
            std::shared_ptr<InputPacket> packet;
            
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                input_condition_.wait(lock, [this] {
                    return !input_queue_.empty() || stop_requested_.load();
                });
                
                if (stop_requested_.load() && input_queue_.empty()) {
                    break;
                }
                
                if (!input_queue_.empty()) {
                    packet = input_queue_.front();
                    input_queue_.pop();
                    stats_.queue_size.store(input_queue_.size());
                }
            }
            
            if (packet) {
                auto start_time = std::chrono::steady_clock::now();
                
                try {
                    auto output = processPacket(packet);
                    if (output) {
                        sendToNextStage(output);
                    }
                } catch (const std::exception& e) {
                    // 记录错误，但继续处理
                    stats_.packets_dropped.fetch_add(1);
                }
                
                auto end_time = std::chrono::steady_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                
                // 更新平均处理时间
                auto processed = stats_.packets_processed.fetch_add(1) + 1;
                auto current_avg = stats_.avg_processing_time.load();
                auto new_avg = (current_avg * (processed - 1) + duration.count() / 1000.0) / processed;
                stats_.avg_processing_time.store(new_avg);
            }
        }
    }
    
protected:
    std::string stage_name_;                              ///< 阶段名称
    size_t max_queue_size_;                               ///< 最大队列大小
    size_t num_workers_;                                  ///< 工作线程数量
    
    std::queue<std::shared_ptr<InputPacket>> input_queue_; ///< 输入队列
    std::mutex queue_mutex_;                              ///< 队列互斥锁
    std::condition_variable input_condition_;             ///< 输入条件变量
    
    std::vector<std::thread> workers_;                    ///< 工作线程
    std::atomic<bool> stop_requested_;                    ///< 停止请求标志
    std::atomic<bool> is_running_;                        ///< 运行状态标志
    
    NextStageCallback next_stage_callback_;               ///< 下一阶段回调函数
    mutable PipelineStats stats_;                         ///< 性能统计
};

} // namespace pipeline
} // namespace core
} // namespace dlogcover

#endif // DLOGCOVER_CORE_PIPELINE_PIPELINE_STAGE_H 