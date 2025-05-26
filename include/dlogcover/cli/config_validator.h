/**
 * @file config_validator.h
 * @brief 配置验证器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CLI_CONFIG_VALIDATOR_H
#define DLOGCOVER_CLI_CONFIG_VALIDATOR_H

#include <dlogcover/cli/error_types.h>
#include <dlogcover/cli/options.h>
#include <dlogcover/cli/config_constants.h>
#include <string>
#include <string_view>

namespace dlogcover {
namespace cli {

/**
 * @brief 配置验证器类
 * 
 * 负责验证配置文件的格式和内容，确保配置的有效性
 */
class ConfigValidator {
public:
    /**
     * @brief 构造函数
     */
    ConfigValidator() = default;

    /**
     * @brief 析构函数
     */
    ~ConfigValidator() = default;

    /**
     * @brief 验证配置文件
     * @param configPath 配置文件路径
     * @return 验证成功返回true，否则返回false
     */
    bool validateConfig(std::string_view configPath);

    /**
     * @brief 从配置文件加载选项
     * @param configPath 配置文件路径
     * @param options 选项对象引用
     * @return 加载成功返回true，否则返回false
     */
    bool loadFromConfig(std::string_view configPath, Options& options);

    /**
     * @brief 从环境变量加载选项
     * @param options 选项对象引用
     * @return 加载成功返回true，否则返回false
     */
    bool loadFromEnvironment(Options& options);

    /**
     * @brief 获取错误代码
     * @return 错误代码
     */
    ConfigError getErrorCode() const { return errorCode_; }

    /**
     * @brief 获取错误消息
     * @return 错误消息
     */
    const std::string& getError() const { return error_; }

    /**
     * @brief 获取配置文件版本
     * @return 配置文件版本
     */
    const std::string& getConfigVersion() const { return configVersion_; }

private:
    ConfigError errorCode_ = ConfigError::None;  ///< 错误代码
    std::string error_;                          ///< 错误消息
    std::string configVersion_;                  ///< 配置文件版本

    /**
     * @brief 设置错误状态
     * @param code 错误代码
     * @param message 错误消息
     */
    void setError(ConfigError code, const std::string& message);

    /**
     * @brief 验证配置文件版本
     * @param version 版本字符串
     * @return 验证成功返回true，否则返回false
     */
    bool validateVersion(std::string_view version);
};

} // namespace cli
} // namespace dlogcover

#endif // DLOGCOVER_CLI_CONFIG_VALIDATOR_H 