/**
 * @file string_utils.cpp
 * @brief 字符串工具函数实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/utils/string_utils.h>
#include <dlogcover/utils/log_utils.h>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <locale>
#include <stdexcept>
#include <memory>
#include <vector>
#include <string_view>

namespace dlogcover {
namespace utils {

std::vector<std::string> split(const std::string& str, const std::string& delim) {
    LOG_DEBUG_FMT("分割字符串，长度: %zu, 分隔符: %s", str.size(), delim.c_str());

    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;

    // 预分配空间以提高性能
    tokens.reserve(std::count(str.begin(), str.end(), delim[0]) + 1);

    do {
        pos = str.find(delim, prev);
        if (pos == std::string::npos) {
            pos = str.length();
        }

        std::string token = str.substr(prev, pos - prev);
        if (!token.empty()) {
            tokens.push_back(token);
        }

        prev = pos + delim.length();
    } while (pos < str.length() && prev < str.length());

    LOG_DEBUG_FMT("分割结果: %zu 个分段", tokens.size());
    return tokens;
}

std::string join(const std::vector<std::string>& vec, const std::string& delim) {
    LOG_DEBUG_FMT("连接字符串，%zu 个分段，分隔符: %s", vec.size(), delim.c_str());

    if (vec.empty()) {
        return "";
    }

    // 预先计算总长度以避免多次分配内存
    size_t totalLength = 0;
    for (const auto& s : vec) {
        totalLength += s.length();
    }
    totalLength += delim.length() * (vec.size() - 1);

    std::string result;
    result.reserve(totalLength);

    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) {
            result.append(delim);
        }
        result.append(vec[i]);
    }

    return result;
}

std::string replace(const std::string& str, const std::string& from, const std::string& to) {
    LOG_DEBUG_FMT("替换字符串，长度: %zu, 替换 '%s' 为 '%s'",
                 str.size(), from.c_str(), to.c_str());

    if (from.empty()) {
        LOG_WARNING("替换操作的目标字符串为空");
        return str;
    }

    std::string result = str;
    size_t start_pos = 0;

    while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
        result.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }

    return result;
}

bool startsWith(const std::string& str, const std::string& prefix) {
    bool result = str.size() >= prefix.size() &&
                 str.compare(0, prefix.size(), prefix) == 0;

    LOG_DEBUG_FMT("检查字符串是否以 '%s' 开头: %s",
                 prefix.c_str(), result ? "是" : "否");

    return result;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    bool result = str.size() >= suffix.size() &&
                 str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;

    LOG_DEBUG_FMT("检查字符串是否以 '%s' 结尾: %s",
                 suffix.c_str(), result ? "是" : "否");

    return result;
}

std::string toLower(const std::string& str) {
    LOG_DEBUG_FMT("转换字符串为小写，长度: %zu", str.size());

    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string toUpper(const std::string& str) {
    LOG_DEBUG_FMT("转换字符串为大写，长度: %zu", str.size());

    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string trim(const std::string& str) {
    LOG_DEBUG_FMT("修剪字符串前后空白，长度: %zu", str.size());
    return trimRight(trimLeft(str));
}

std::string trimLeft(const std::string& str) {
    LOG_DEBUG_FMT("修剪字符串左侧空白，长度: %zu", str.size());

    std::string result = str;
    result.erase(result.begin(),
                std::find_if(result.begin(), result.end(),
                            [](unsigned char ch) {
                                return !std::isspace(ch);
                            }));
    return result;
}

std::string trimRight(const std::string& str) {
    LOG_DEBUG_FMT("修剪字符串右侧空白，长度: %zu", str.size());

    std::string result = str;
    result.erase(std::find_if(result.rbegin(), result.rend(),
                             [](unsigned char ch) {
                                 return !std::isspace(ch);
                             }).base(),
                result.end());
    return result;
}

std::string format(const char* fmt, ...) {
    if (!fmt) {
        LOG_ERROR("格式化字符串时传入了空格式");
        return "";
    }

    LOG_DEBUG_FMT("格式化字符串: %s", fmt);

    // 使用更安全的格式化方法，避免缓冲区溢出
    va_list args1;
    va_start(args1, fmt);

    // 首先计算所需的缓冲区大小
    va_list args2;
    va_copy(args2, args1);
    int size = vsnprintf(nullptr, 0, fmt, args2);
    va_end(args2);

    if (size < 0) {
        LOG_ERROR("格式化字符串失败");
        va_end(args1);
        return "";
    }

    // 分配适当大小的缓冲区，+1 是为了 null 终止符
    std::vector<char> buffer(size + 1);

    // 执行实际的格式化
    vsnprintf(buffer.data(), buffer.size(), fmt, args1);
    va_end(args1);

    // 创建并返回格式化后的字符串
    return std::string(buffer.data(), size);
}

bool tryParseInt(const std::string& str, int& value) {
    LOG_DEBUG_FMT("尝试解析整数: %s", str.c_str());

    if (str.empty()) {
        LOG_WARNING("尝试解析空字符串为整数");
        return false;
    }

    try {
        size_t pos = 0;
        value = std::stoi(str, &pos);
        bool success = pos == str.length();
        LOG_DEBUG_FMT("整数解析%s: %s -> %d",
                     success ? "成功" : "失败", str.c_str(), value);
        return success;
    } catch (const std::exception& e) {
        LOG_WARNING_FMT("解析整数失败: %s, 错误: %s", str.c_str(), e.what());
        return false;
    }
}

bool tryParseDouble(const std::string& str, double& value) {
    LOG_DEBUG_FMT("尝试解析浮点数: %s", str.c_str());

    if (str.empty()) {
        LOG_WARNING("尝试解析空字符串为浮点数");
        return false;
    }

    try {
        size_t pos = 0;
        value = std::stod(str, &pos);
        bool success = pos == str.length();
        LOG_DEBUG_FMT("浮点数解析%s: %s -> %f",
                     success ? "成功" : "失败", str.c_str(), value);
        return success;
    } catch (const std::exception& e) {
        LOG_WARNING_FMT("解析浮点数失败: %s, 错误: %s", str.c_str(), e.what());
        return false;
    }
}

std::string repeat(const std::string& str, int count) {
    LOG_DEBUG_FMT("重复字符串 %d 次, 原长度: %zu", count, str.size());

    if (count <= 0 || str.empty()) {
        return {};
    }

    std::string result;
    result.reserve(str.size() * count);

    for (int i = 0; i < count; ++i) {
        result += str;
    }

    return result;
}

bool containsSubstring(const std::string& str, const std::string& sub) {
    LOG_DEBUG_FMT("检查字符串是否包含子串: %s", sub.c_str());

    bool contains = str.find(sub) != std::string::npos;
    LOG_DEBUG_FMT("字符串%s包含子串 '%s'",
                 contains ? "" : "不", sub.c_str());

    return contains;
}

std::string replaceAll(const std::string& str, const std::map<std::string, std::string>& replacements) {
    LOG_DEBUG_FMT("批量替换字符串中的 %zu 种模式", replacements.size());

    std::string result = str;

    for (const auto& [from, to] : replacements) {
        result = replace(result, from, to);
    }

    return result;
}

std::string to_utf8(const std::string& str) {
    std::string utf8_string;
    utf8_string.reserve(str.length());

    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = str[i];
        
        if (c < 0x80) { // 0xxxxxxx
            utf8_string += c;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) { // 110xxxxx 10xxxxxx
            if (i + 1 < str.length() && (str[i+1] & 0xC0) == 0x80) {
                utf8_string += c;
                utf8_string += str[i+1];
                i += 2;
            } else {
                utf8_string += '?';
                i += 1;
            }
        } else if ((c & 0xF0) == 0xE0) { // 1110xxxx 10xxxxxx 10xxxxxx
            if (i + 2 < str.length() && (str[i+1] & 0xC0) == 0x80 && (str[i+2] & 0xC0) == 0x80) {
                utf8_string += c;
                utf8_string += str[i+1];
                utf8_string += str[i+2];
                i += 3;
            } else {
                utf8_string += '?';
                i += 1;
            }
        } else if ((c & 0xF8) == 0xF0) { // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
             if (i + 3 < str.length() && (str[i+1] & 0xC0) == 0x80 && (str[i+2] & 0xC0) == 0x80 && (str[i+3] & 0xC0) == 0x80) {
                utf8_string += c;
                utf8_string += str[i+1];
                utf8_string += str[i+2];
                utf8_string += str[i+3];
                i += 4;
            } else {
                utf8_string += '?';
                i += 1;
            }
        }
        else {
            utf8_string += '?';
            i += 1;
        }
    }
    return utf8_string;
}

} // namespace utils
} // namespace dlogcover
