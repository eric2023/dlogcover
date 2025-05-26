/**
 * @file reporter_types.h
 * @brief 报告生成器类型定义
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_REPORTER_REPORTER_TYPES_H
#define DLOGCOVER_REPORTER_REPORTER_TYPES_H

#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <string>
#include <algorithm>

namespace dlogcover {
namespace reporter {

/**
 * @brief 报告格式枚举
 */
enum class ReportFormat {
    TEXT = 0,       ///< 文本格式
    JSON = 1,       ///< JSON格式
    HTML = 2,       ///< HTML格式
    XML = 3         ///< XML格式
};

/**
 * @brief 报告生成器错误类型
 */
enum class ReporterError {
    NONE = 0,               ///< 无错误
    INVALID_PATH = 1,       ///< 无效路径
    FILE_WRITE_ERROR = 2,   ///< 文件写入错误
    FILE_ERROR = 3,         ///< 文件错误
    DIRECTORY_ERROR = 4,    ///< 目录错误
    FORMAT_ERROR = 5,       ///< 格式错误
    GENERATION_ERROR = 6,   ///< 生成错误
    INTERNAL_ERROR = 7      ///< 内部错误
};

/**
 * @brief 结果类型别名
 */
template<typename T>
using Result = core::ast_analyzer::Result<T>;

/**
 * @brief 创建成功结果的辅助函数
 */
template<typename T>
Result<T> makeSuccess(T&& value) {
    return core::ast_analyzer::makeSuccess(std::forward<T>(value));
}

/**
 * @brief 创建错误结果的辅助函数
 */
template<typename T>
Result<T> makeError(ReporterError error, const std::string& message) {
    return core::ast_analyzer::makeError<T>(core::ast_analyzer::ASTAnalyzerError::INTERNAL_ERROR, message);
}

/**
 * @brief 报告格式转字符串
 * @param format 报告格式
 * @return 对应的字符串表示
 */
inline std::string getReportFormatString(ReportFormat format) {
    switch (format) {
        case ReportFormat::TEXT:
            return "text";
        case ReportFormat::JSON:
            return "json";
        case ReportFormat::HTML:
            return "html";
        case ReportFormat::XML:
            return "xml";
        default:
            return "text";
    }
}

/**
 * @brief 字符串转报告格式
 * @param str 字符串
 * @return 对应的报告格式
 */
inline ReportFormat parseReportFormat(const std::string& str) {
    std::string strLower = str;
    std::transform(strLower.begin(), strLower.end(), strLower.begin(), ::tolower);
    
    if (strLower == "text") {
        return ReportFormat::TEXT;
    } else if (strLower == "json") {
        return ReportFormat::JSON;
    } else if (strLower == "html") {
        return ReportFormat::HTML;
    } else if (strLower == "xml") {
        return ReportFormat::XML;
    } else {
        return ReportFormat::TEXT; // 默认返回TEXT格式
    }
}

} // namespace reporter
} // namespace dlogcover

#endif // DLOGCOVER_REPORTER_REPORTER_TYPES_H 