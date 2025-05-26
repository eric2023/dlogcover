/**
 * @file config_manager.h
 * @brief 配置管理器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CONFIG_CONFIG_MANAGER_H
#define DLOGCOVER_CONFIG_CONFIG_MANAGER_H

#include <dlogcover/config/config.h>
#include <dlogcover/cli/options.h>
#include <string>

// 前向声明
namespace nlohmann {
class json;
}

namespace dlogcover {
namespace config {

/**
 * @brief 配置管理器类
 * 
 * 负责加载、解析、验证和管理配置信息
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
     * @param path 配置文件路径
     * @return 加载是否成功
     */
    bool loadConfig(const std::string& path);

    /**
     * @brief 与命令行选项合并
     * @param options 命令行选项
     */
    void mergeWithCommandLineOptions(const cli::Options& options);

    /**
     * @brief 获取配置
     * @return 配置对象的常量引用
     */
    const Config& getConfig() const;

    /**
     * @brief 获取默认配置
     * @return 默认配置对象
     */
    static Config getDefaultConfig();

    /**
     * @brief 验证配置
     * @return 配置是否有效
     */
    bool validateConfig() const;

private:
    Config config_;  ///< 配置对象

    /**
     * @brief 解析扫描配置
     * @param jsonConfig JSON配置对象
     * @return 解析是否成功
     */
    bool parseScanConfig(const nlohmann::json& jsonConfig);

    /**
     * @brief 解析日志函数配置
     * @param jsonConfig JSON配置对象
     * @return 解析是否成功
     */
    bool parseLogFunctionsConfig(const nlohmann::json& jsonConfig);

    /**
     * @brief 解析分析配置
     * @param jsonConfig JSON配置对象
     * @return 解析是否成功
     */
    bool parseAnalysisConfig(const nlohmann::json& jsonConfig);

    /**
     * @brief 解析报告配置
     * @param jsonConfig JSON配置对象
     * @return 解析是否成功
     */
    bool parseReportConfig(const nlohmann::json& jsonConfig);

    /**
     * @brief 验证扫描配置
     * @return 配置是否有效
     */
    bool validateScanConfig() const;

    /**
     * @brief 验证日志函数配置
     * @return 配置是否有效
     */
    bool validateLogFunctionsConfig() const;

    /**
     * @brief 验证报告配置
     * @return 配置是否有效
     */
    bool validateReportConfig() const;

    /**
     * @brief 更新日志级别过滤器
     * @param level 日志级别
     */
    void updateLogLevelFilters(cli::LogLevel level);
};

} // namespace config
} // namespace dlogcover

#endif // DLOGCOVER_CONFIG_CONFIG_MANAGER_H 