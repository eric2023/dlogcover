/**
 * @file reporter_types.h
 * @brief 报告生成器类型定义
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_REPORTER_REPORTER_TYPES_H
#define DLOGCOVER_REPORTER_REPORTER_TYPES_H

#include <dlogcover/core/ast_analyzer/ast_types.h>
#include <string>

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
    DIRECTORY_ERROR = 3,    ///< 目录错误
    FORMAT_ERROR = 4,       ///< 格式错误
    INTERNAL_ERROR = 5      ///< 内部错误
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
std::string getReportFormatString(ReportFormat format);

/**
 * @brief 字符串转报告格式
 * @param str 字符串
 * @return 对应的报告格式
 */
ReportFormat parseReportFormat(const std::string& str);

} // namespace reporter
} // namespace dlogcover

#endif // DLOGCOVER_REPORTER_REPORTER_TYPES_H 