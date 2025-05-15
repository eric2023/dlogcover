/**
 * @file string_utils.cpp
 * @brief 字符串工具函数实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/utils/string_utils.h>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <locale>
#include <stdexcept>

namespace dlogcover {
namespace utils {

std::vector<std::string> split(const std::string& str, const std::string& delim) {
    std::vector<std::string> tokens;
    size_t prev = 0, pos = 0;
    
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
    
    return tokens;
}

std::string join(const std::vector<std::string>& vec, const std::string& delim) {
    std::stringstream ss;
    
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) {
            ss << delim;
        }
        ss << vec[i];
    }
    
    return ss.str();
}

std::string replace(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t start_pos = 0;
    
    while ((start_pos = result.find(from, start_pos)) != std::string::npos) {
        result.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    
    return result;
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && 
           str.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                  [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                  [](unsigned char c) { return std::toupper(c); });
    return result;
}

std::string trim(const std::string& str) {
    return trimRight(trimLeft(str));
}

std::string trimLeft(const std::string& str) {
    std::string result = str;
    result.erase(result.begin(), 
                std::find_if(result.begin(), result.end(), 
                            [](unsigned char ch) { 
                                return !std::isspace(ch); 
                            }));
    return result;
}

std::string trimRight(const std::string& str) {
    std::string result = str;
    result.erase(std::find_if(result.rbegin(), result.rend(), 
                             [](unsigned char ch) { 
                                 return !std::isspace(ch); 
                             }).base(), 
                result.end());
    return result;
}

std::string format(const char* fmt, ...) {
    const int initialSize = 256;
    char buffer[initialSize];
    va_list args;
    
    va_start(args, fmt);
    int result = vsnprintf(buffer, initialSize, fmt, args);
    va_end(args);
    
    if (result < 0) {
        return {};
    }
    
    if (result < initialSize) {
        return std::string(buffer, result);
    }
    
    // 如果初始缓冲区不够大，重新分配并重试
    std::vector<char> largerBuffer(result + 1);
    
    va_start(args, fmt);
    result = vsnprintf(largerBuffer.data(), largerBuffer.size(), fmt, args);
    va_end(args);
    
    if (result < 0) {
        return {};
    }
    
    return std::string(largerBuffer.data(), result);
}

bool tryParseInt(const std::string& str, int& value) {
    try {
        size_t pos = 0;
        value = std::stoi(str, &pos);
        return pos == str.length();
    } catch (const std::exception&) {
        return false;
    }
}

bool tryParseDouble(const std::string& str, double& value) {
    try {
        size_t pos = 0;
        value = std::stod(str, &pos);
        return pos == str.length();
    } catch (const std::exception&) {
        return false;
    }
}

std::string repeat(const std::string& str, int count) {
    if (count <= 0) {
        return {};
    }
    
    std::string result;
    result.reserve(str.size() * count);
    
    for (int i = 0; i < count; ++i) {
        result += str;
    }
    
    return result;
}

} // namespace utils
} // namespace dlogcover 