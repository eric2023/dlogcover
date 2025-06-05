/**
 * @file log_types.cpp
 * @brief 统一的日志类型定义实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/common/log_types.h>
#include <algorithm>
#include <stdexcept>

namespace dlogcover {
namespace common {

std::string_view toString(LogLevel level) {
    // 使用if-else而不是switch来处理ERROR和CRITICAL的相同值
    if (level == LogLevel::UNKNOWN) {
        return "unknown";
    } else if (level == LogLevel::DEBUG) {
        return "debug";
    } else if (level == LogLevel::INFO) {
        return "info";
    } else if (level == LogLevel::WARNING) {
        return "warning";
    } else if (level == LogLevel::ERROR || level == LogLevel::CRITICAL) {
        // ERROR和CRITICAL都返回"error"，但可以根据需要区分
        return "error";
    } else if (level == LogLevel::FATAL) {
        return "fatal";
    } else if (level == LogLevel::ALL) {
        return "all";
    } else {
        return "unknown";
    }
}

LogLevel parseLogLevel(std::string_view str) {
    // 转换为小写进行比较
    std::string strLower{str};
    std::transform(strLower.begin(), strLower.end(), strLower.begin(), ::tolower);
    
    if (strLower == "unknown") {
        return LogLevel::UNKNOWN;
    } else if (strLower == "debug") {
        return LogLevel::DEBUG;
    } else if (strLower == "info") {
        return LogLevel::INFO;
    } else if (strLower == "warning" || strLower == "warn") {
        return LogLevel::WARNING;
    } else if (strLower == "error") {
        return LogLevel::ERROR;
    } else if (strLower == "critical") {
        return LogLevel::CRITICAL;
    } else if (strLower == "fatal") {
        return LogLevel::FATAL;
    } else if (strLower == "all") {
        return LogLevel::ALL;
    } else {
        throw std::invalid_argument("Invalid log level: " + std::string(str));
    }
}

bool isValidLogLevel(LogLevel level) {
    return level >= LogLevel::UNKNOWN && level <= LogLevel::ALL;
}

bool shouldLog(LogLevel messageLevel, LogLevel currentLevel) {
    // ALL级别表示输出所有日志
    if (currentLevel == LogLevel::ALL) {
        return true;
    }
    
    // 消息级别必须大于等于当前设置的级别
    return messageLevel >= currentLevel;
}

LogLevel getDefaultLogLevel() {
    return LogLevel::INFO;
}

} // namespace common
} // namespace dlogcover 