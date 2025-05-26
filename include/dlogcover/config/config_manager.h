/**
 * @file config_manager.h
 * @brief 配置管理器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CONFIG_CONFIG_MANAGER_H
#define DLOGCOVER_CONFIG_CONFIG_MANAGER_H

#include <dlogcover/config/config.h>
#include <dlogcover/cli/options.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// 前向声明
namespace dlogcover {
namespace cli {
    class Options;
    enum class LogLevel;
}
}

namespace dlogcover {
namespace config {

/**
 * @brief 配置管理器类
 * 
 * 负责加载、解析和管理配置文件
 */
class ConfigManager {
public:
    /**
     * @brief 构造函数
     */
    ConfigManager();

    /**
     * @brief 析构函数
     */
    ~ConfigManager();

    /**
     * @brief 加载配置文件
     * @param configPath 配置文件路径
     * @return 加载成功返回true，否则返回false
     */
    bool loadConfig(const std::string& configPath);

    /**
     * @brief 获取配置对象
     * @return 配置对象的常量引用
     */
    const Config& getConfig() const;

    /**
     * @brief 检查配置文件是否已加载
     * @return 如果已加载返回true，否则返回false
     */
    bool isLoaded() const { return loaded_; }

    /**
     * @brief 获取错误消息
     * @return 错误消息
     */
    const std::string& getError() const { return error_; }

    /**
     * @brief 重置配置管理器
     */
    void reset();

    /**
     * @brief 验证配置文件格式
     * @param configPath 配置文件路径
     * @return 验证成功返回true，否则返回false
     */
    bool validateConfig(const std::string& configPath);

    /**
     * @brief 验证配置对象
     * @return 验证成功返回true，否则返回false
     */
    bool validateConfig() const;

    /**
     * @brief 与命令行选项合并
     * @param options 命令行选项
     */
    void mergeWithCommandLineOptions(const dlogcover::cli::Options& options);

    /**
     * @brief 更新日志级别过滤器
     * @param level 日志级别
     */
    void updateLogLevelFilters(dlogcover::cli::LogLevel level);

    /**
     * @brief 获取默认配置
     * @return 默认配置对象
     */
    static Config getDefaultConfig();

    // 便捷访问器方法
    /**
     * @brief 获取日志函数配置
     * @return 日志函数配置的常量引用
     */
    const LogFunctionsConfig& getLogFunctionsConfig() const { return config_.logFunctions; }

    /**
     * @brief 获取扫描配置
     * @return 扫描配置的常量引用
     */
    const ScanConfig& getScanConfig() const { return config_.scan; }

    /**
     * @brief 获取分析配置
     * @return 分析配置的常量引用
     */
    const AnalysisConfig& getAnalysisConfig() const { return config_.analysis; }

    /**
     * @brief 获取报告配置
     * @return 报告配置的常量引用
     */
    const ReportConfig& getReportConfig() const { return config_.report; }

    // 兼容性访问器 - 简化的属性访问方式
    struct {
        const QtLogConfig& qt;
        const CustomLogConfig& custom;
        
        // 构造函数
        explicit operator const LogFunctionsConfig&() const {
            return *reinterpret_cast<const LogFunctionsConfig*>(this);
        }
    } logFunctions;

private:
    Config config_;                     ///< 配置对象
    bool loaded_ = false;               ///< 是否已加载
    std::string error_;                 ///< 错误消息

    /**
     * @brief 解析扫描配置
     * @param jsonConfig JSON配置对象
     * @return 解析成功返回true，否则返回false
     */
    bool parseScanConfig(const nlohmann::json& jsonConfig);

    /**
     * @brief 设置错误消息
     * @param message 错误消息
     */
    void setError(const std::string& message);

    /**
     * @brief 解析日志函数配置
     * @param jsonConfig JSON配置对象
     * @return 解析成功返回true，否则返回false
     */
    bool parseLogFunctionsConfig(const nlohmann::json& jsonConfig);

    /**
     * @brief 解析分析配置
     * @param jsonConfig JSON配置对象
     * @return 解析成功返回true，否则返回false
     */
    bool parseAnalysisConfig(const nlohmann::json& jsonConfig);

    /**
     * @brief 解析报告配置
     * @param jsonConfig JSON配置对象
     * @return 解析成功返回true，否则返回false
     */
    bool parseReportConfig(const nlohmann::json& jsonConfig);

    /**
     * @brief 验证扫描配置
     * @return 验证成功返回true，否则返回false
     */
    bool validateScanConfig() const;

    /**
     * @brief 验证日志函数配置
     * @return 验证成功返回true，否则返回false
     */
    bool validateLogFunctionsConfig() const;

    /**
     * @brief 验证报告配置
     * @return 验证成功返回true，否则返回false
     */
    bool validateReportConfig() const;
};

} // namespace config
} // namespace dlogcover

#endif // DLOGCOVER_CONFIG_CONFIG_MANAGER_H 