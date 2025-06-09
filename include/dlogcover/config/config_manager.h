/**
 * @file config_manager.h
 * @brief 配置管理器 - 基于compile_commands.json的简化配置管理
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CONFIG_CONFIG_MANAGER_H
#define DLOGCOVER_CONFIG_CONFIG_MANAGER_H

#include <dlogcover/config/config.h>
#include <dlogcover/config/compile_commands_manager.h>
#include <dlogcover/cli/options.h>
#include <string>
#include <memory>

namespace dlogcover {
namespace config {

/**
 * @brief 配置管理器类
 * 
 * 负责加载、解析和管理配置文件，基于compile_commands.json的简化配置方案
 */
class ConfigManager {
public:
    /**
     * @brief 默认构造函数
     * 
     * 初始化时将加载程序内置的默认配置
     */
    ConfigManager();

    /**
     * @brief 析构函数
     */
    ~ConfigManager();

    /**
     * @brief 加载配置文件
     * @param config_path 配置文件路径
     * @return 加载成功返回true，否则返回false
     */
    bool loadConfig(const std::string& config_path);

    /**
     * @brief 初始化默认配置
     * @param project_dir 项目目录（可选，默认为当前目录）
     * @return 初始化成功返回true，否则返回false
     */
    bool initializeDefault(const std::string& project_dir = ".");

    /**
     * @brief 获取配置对象
     * @return 配置对象的常量引用
     */
    const Config& getConfig() const;

    /**
     * @brief 获取编译命令管理器
     * @return 编译命令管理器的引用
     */
    CompileCommandsManager& getCompileCommandsManager();

    /**
     * @brief 检查配置是否已加载
     * @return 如果已加载返回true，否则返回false
     */
    bool isLoaded() const { return loaded_; }

    /**
     * @brief 获取错误消息
     * @return 错误消息
     */
    const std::string& getError() const { return error_; }

    /**
     * @brief 验证配置
     * @return 验证成功返回true，否则返回false
     */
    bool validateConfig() const;

    /**
     * @brief 与命令行选项合并
     * @param options 命令行选项
     */
    void mergeWithCommandLineOptions(const dlogcover::cli::Options& options);

    /**
     * @brief 生成默认配置文件
     * @param config_path 配置文件路径
     * @param project_dir 项目目录
     * @return 生成成功返回true，否则返回false
     */
    static bool generateDefaultConfig(const std::string& config_path, 
                                     const std::string& project_dir = ".");

    /**
     * @brief 获取默认配置
     * @param project_dir 项目目录
     * @return 默认配置对象
     */
    static Config createDefaultConfig(const std::string& project_dir = ".");

    // 便捷访问器方法
    const ProjectConfig& getProjectConfig() const { return config_.project; }
    const ScanConfig& getScanConfig() const { return config_.scan; }
    const CompileCommandsConfig& getCompileCommandsConfig() const { return config_.compile_commands; }
    const OutputConfig& getOutputConfig() const { return config_.output; }
    const LogFunctionsConfig& getLogFunctionsConfig() const { return config_.log_functions; }
    const AnalysisConfig& getAnalysisConfig() const { return config_.analysis; }

private:
    Config config_;                                           ///< 配置对象
    std::unique_ptr<CompileCommandsManager> compile_manager_; ///< 编译命令管理器
    bool loaded_ = false;                                     ///< 是否已加载
    std::string error_;                                       ///< 错误消息

    /**
     * @brief 解析JSON配置文件
     * @param config_path 配置文件路径
     * @return 解析成功返回true，否则返回false
     */
    bool parseConfigFile(const std::string& config_path);

    /**
     * @brief 设置错误消息
     * @param message 错误消息
     */
    void setError(const std::string& message);

    /**
     * @brief 验证项目配置
     * @return 验证成功返回true，否则返回false
     */
    bool validateProjectConfig() const;

    /**
     * @brief 验证扫描配置
     * @return 验证成功返回true，否则返回false
     */
    bool validateScanConfig() const;

    /**
     * @brief 验证编译命令配置
     * @return 验证成功返回true，否则返回false
     */
    bool validateCompileCommandsConfig() const;
};

} // namespace config
} // namespace dlogcover

#endif // DLOGCOVER_CONFIG_CONFIG_MANAGER_H 