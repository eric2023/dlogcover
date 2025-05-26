/**
 * @file result.h
 * @brief 通用结果类型定义
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_COMMON_RESULT_H
#define DLOGCOVER_COMMON_RESULT_H

#include <string>
#include <utility>

namespace dlogcover {

/**
 * @brief 通用结果类型，用于表示操作的成功或失败
 * @tparam T 成功时返回的值类型
 * @tparam E 错误类型
 */
template<typename T, typename E = int>
class Result {
public:
    /**
     * @brief 构造成功结果
     * @param value 成功值
     */
    explicit Result(T&& value) : hasError_(false), value_(std::move(value)) {}
    
    /**
     * @brief 构造成功结果
     * @param value 成功值
     */
    explicit Result(const T& value) : hasError_(false), value_(value) {}

    /**
     * @brief 构造错误结果
     * @param error 错误代码
     * @param message 错误消息
     */
    Result(E error, const std::string& message) 
        : hasError_(true), error_(error), errorMessage_(message) {}

    /**
     * @brief 检查是否有错误
     * @return 如果有错误返回true，否则返回false
     */
    bool hasError() const { return hasError_; }

    /**
     * @brief 获取成功值
     * @return 成功值的引用
     */
    T& value() { return value_; }

    /**
     * @brief 获取成功值（常量版本）
     * @return 成功值的常量引用
     */
    const T& value() const { return value_; }

    /**
     * @brief 获取错误代码
     * @return 错误代码
     */
    E error() const { return error_; }

    /**
     * @brief 获取错误消息
     * @return 错误消息
     */
    const std::string& errorMessage() const { return errorMessage_; }

private:
    bool hasError_;
    T value_;
    E error_;
    std::string errorMessage_;
};

/**
 * @brief 创建成功结果的辅助函数
 * @tparam T 值类型
 * @param value 成功值
 * @return 成功结果
 */
template<typename T>
Result<T> makeSuccess(T&& value) {
    return Result<T>(std::forward<T>(value));
}

/**
 * @brief 创建错误结果的辅助函数
 * @tparam T 值类型
 * @tparam E 错误类型
 * @param error 错误代码
 * @param message 错误消息
 * @return 错误结果
 */
template<typename T, typename E>
Result<T, E> makeError(E error, const std::string& message) {
    return Result<T, E>(error, message);
}

} // namespace dlogcover

#endif // DLOGCOVER_COMMON_RESULT_H 