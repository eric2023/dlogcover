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
    ~CommandLineParser();

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
     * @brief 检查是否是帮助请求
     * @return 如果是帮助请求返回true
     */
    bool isHelpRequest() const;

    /**
     * @brief 检查是否是版本请求  
     * @return 如果是版本请求返回true
     */
    bool isVersionRequest() const;

    /**
     * @brief 显示帮助信息
     */
    void showHelp() const;

    /**
     * @brief 显示版本信息
     */
    void showVersion() const;

    /**
     * @brief 将解析后的参数输出到日志文件
     * @details 该函数需要在日志系统初始化后调用，用于记录命令行参数解析结果
     */
    void logParsedOptions() const;

private:
    Options options_;                    ///< 解析后的选项
    bool isHelpRequest_;                 ///< 是否是帮助请求
    bool isVersionRequest_;              ///< 是否是版本请求

    /**
     * @brief 处理选项的通用模板方法
     * @tparam Handler 处理器类型
     * @param args 参数数组
     * @param i 当前参数索引
     * @param arg 当前参数
     * @param handler 处理器函数
     * @return 处理结果
     */
    template<typename Handler>
    ErrorResult handleOption(const std::vector<std::string>& args, 
                           size_t& i, 
                           const std::string_view& arg, 
                           Handler handler);

    /**
     * @brief 验证路径有效性
     * @param path 路径
     * @param isDirectory 是否为目录
     * @return 验证结果
     */
    ErrorResult validatePath(std::string_view path, bool isDirectory);
};

// 模板方法的实现
template<typename Handler>
ErrorResult CommandLineParser::handleOption(const std::vector<std::string>& args, 
                                           size_t& i, 
                                           const std::string_view& arg, 
                                           Handler handler) {
    if (i + 1 >= args.size()) {
        return ErrorResult(ConfigError::MISSING_VALUE, 
                          "选项缺少值: " + std::string(arg));
    }
    ++i;
    return handler(args[i]);
}

} // namespace cli
} // namespace dlogcover

#endif // DLOGCOVER_CLI_COMMAND_LINE_PARSER_H 