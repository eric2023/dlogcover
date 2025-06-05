/**
 * @file identifier_types.cpp
 * @brief 日志识别器类型定义实现
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <dlogcover/core/log_identifier/identifier_types.h>
#include <dlogcover/common/log_types.h>
#include <algorithm>
#include <stdexcept>

namespace dlogcover {
namespace core {
namespace log_identifier {

std::string toString(LogLevel level) {
    // 直接使用common命名空间的toString函数
    return std::string(common::toString(level));
}

LogLevel parseLogLevel(const std::string& str) {
    // 直接使用common命名空间的parseLogLevel函数
    return common::parseLogLevel(str);
}

std::string toString(LogType type) {
    switch (type) {
        case LogType::UNKNOWN:
            return "unknown";
        case LogType::QT:
            return "qt";
        case LogType::CUSTOM:
            return "custom";
        case LogType::QT_CATEGORY:
            return "qt_category";
        default:
            return "unknown";
    }
}

LogType parseLogType(const std::string& str) {
    std::string strLower = str;
    std::transform(strLower.begin(), strLower.end(), strLower.begin(), ::tolower);
    
    if (strLower == "unknown") {
        return LogType::UNKNOWN;
    } else if (strLower == "qt") {
        return LogType::QT;
    } else if (strLower == "custom") {
        return LogType::CUSTOM;
    } else if (strLower == "qt_category") {
        return LogType::QT_CATEGORY;
    } else {
        throw std::invalid_argument("Invalid log type: " + str);
    }
}

} // namespace log_identifier
} // namespace core
} // namespace dlogcover 