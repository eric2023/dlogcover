/**
 * @file string_utils.h
 * @brief 字符串工具函数
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_UTILS_STRING_UTILS_H
#define DLOGCOVER_UTILS_STRING_UTILS_H

#include <string>
#include <vector>
#include <map>

namespace dlogcover {
namespace utils {

/**
 * @brief 分割字符串
 * @param str 要分割的字符串
 * @param delim 分隔符
 * @return 分割后的字符串数组
 */
std::vector<std::string> split(const std::string& str, const std::string& delim);

/**
 * @brief 连接字符串数组
 * @param vec 字符串数组
 * @param delim 分隔符
 * @return 连接后的字符串
 */
std::string join(const std::vector<std::string>& vec, const std::string& delim);

/**
 * @brief 替换字符串中的所有匹配项
 * @param str 原字符串
 * @param from 要替换的字符串
 * @param to 替换为的字符串
 * @return 替换后的字符串
 */
std::string replace(const std::string& str, const std::string& from, const std::string& to);

/**
 * @brief 检查字符串是否以指定前缀开始
 * @param str 要检查的字符串
 * @param prefix 前缀
 * @return 如果以指定前缀开始返回true，否则返回false
 */
bool startsWith(const std::string& str, const std::string& prefix);

/**
 * @brief 检查字符串是否以指定后缀结束
 * @param str 要检查的字符串
 * @param suffix 后缀
 * @return 如果以指定后缀结束返回true，否则返回false
 */
bool endsWith(const std::string& str, const std::string& suffix);

/**
 * @brief 将字符串转换为小写
 * @param str 原字符串
 * @return 小写字符串
 */
std::string toLower(const std::string& str);

/**
 * @brief 将字符串转换为大写
 * @param str 原字符串
 * @return 大写字符串
 */
std::string toUpper(const std::string& str);

/**
 * @brief 去除字符串前后的空白字符
 * @param str 原字符串
 * @return 去除空白后的字符串
 */
std::string trim(const std::string& str);

/**
 * @brief 去除字符串左侧的空白字符
 * @param str 原字符串
 * @return 去除左侧空白后的字符串
 */
std::string trimLeft(const std::string& str);

/**
 * @brief 去除字符串右侧的空白字符
 * @param str 原字符串
 * @return 去除右侧空白后的字符串
 */
std::string trimRight(const std::string& str);

/**
 * @brief 格式化字符串
 * @param fmt 格式字符串
 * @param ... 格式参数
 * @return 格式化后的字符串
 */
std::string format(const char* fmt, ...);

/**
 * @brief 尝试将字符串解析为整数
 * @param str 要解析的字符串
 * @param value 输出的整数值
 * @return 解析成功返回true，否则返回false
 */
bool tryParseInt(const std::string& str, int& value);

/**
 * @brief 尝试将字符串解析为双精度浮点数
 * @param str 要解析的字符串
 * @param value 输出的双精度浮点数值
 * @return 解析成功返回true，否则返回false
 */
bool tryParseDouble(const std::string& str, double& value);

/**
 * @brief 重复字符串指定次数
 * @param str 要重复的字符串
 * @param count 重复次数
 * @return 重复后的字符串
 */
std::string repeat(const std::string& str, int count);

/**
 * @brief 检查字符串是否包含子字符串
 * @param str 要检查的字符串
 * @param sub 子字符串
 * @return 如果包含返回true，否则返回false
 */
bool containsSubstring(const std::string& str, const std::string& sub);

/**
 * @brief 批量替换字符串
 * @param str 原字符串
 * @param replacements 替换映射表（键为要替换的字符串，值为替换后的字符串）
 * @return 替换后的字符串
 */
std::string replaceAll(const std::string& str, const std::map<std::string, std::string>& replacements);

/**
 * @brief 将字符串安全地转换为UTF-8编码
 * 
 * @param str 输入字符串
 * @return std::string UTF-8编码的字符串
 */
std::string to_utf8(const std::string& str);

} // namespace utils
} // namespace dlogcover

#endif // DLOGCOVER_UTILS_STRING_UTILS_H 