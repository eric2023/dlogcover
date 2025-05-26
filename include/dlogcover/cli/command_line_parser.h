/**
 * @file command_line_parser.h
 * @brief 命令行解析器
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_CLI_COMMAND_LINE_PARSER_H
#define DLOGCOVER_CLI_COMMAND_LINE_PARSER_H

#include <dlogcover/cli/options.h>
#include <string>
#include <vector>
#include <functional>
#include <string_view>

namespace dlogcover {
namespace cli {

/**
 * @brief 命令行解析器类
 * 
 * 负责解析命令行参数并生成选项对象
 */
class CommandLineParser {
public:
    /**
     * @brief 构造函数
     */
    CommandLineParser();

    /**
     * @brief 析构函数
     */
    ~CommandLineParser() = default;

    /**
     * @brief 解析命令行参数
     * @param argc 参数个数
     * @param argv 参数数组
     * @return 解析结果
     */
    ErrorResult parse(int argc, char** argv);

    /**
     * @brief 获取解析后的选项
     * @return 选项对象的常量引用
     */
    const Options& getOptions() const;

    /**
     * @brief 检查是否是帮助或版本请求
     * @return 如果是帮助或版本请求返回true
     */
    bool isHelpOrVersionRequest() const;

    /**
     * @brief 显示帮助信息
     */
    static void showHelp();

    /**
     * @brief 显示版本信息
     */
    static void showVersion();

private:
    Options options_;                    ///< 解析后的选项
    bool isHelpRequest_;                 ///< 是否是帮助请求
    bool isVersionRequest_;              ///< 是否是版本请求

    /**
     * @brief 处理选项参数
     * @tparam Handler 处理函数类型
     * @param args 参数列表
     * @param index 当前参数索引
     * @param option 选项名称
     * @param handler 处理函数
     * @return 处理结果
     */
    template<typename Handler>
    ErrorResult handleOption(const std::vector<std::string>& args, size_t& index, 
                           const std::string& option, Handler handler);

    /**
     * @brief 验证路径有效性
     * @param path 路径
     * @param isDirectory 是否为目录
     * @return 验证结果
     */
    ErrorResult validatePath(std::string_view path, bool isDirectory);

    /**
     * @brief 从环境变量加载选项
     */
    void loadFromEnvironment();

    /**
     * @brief 设置默认值
     */
    void setDefaults();

    /**
     * @brief 生成默认输出路径
     * @return 默认输出路径
     */
    std::string generateDefaultOutputPath() const;

    /**
     * @brief 生成默认日志路径
     * @return 默认日志路径
     */
    std::string generateDefaultLogPath() const;
};

} // namespace cli
} // namespace dlogcover

#endif // DLOGCOVER_CLI_COMMAND_LINE_PARSER_H 