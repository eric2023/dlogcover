/**
 * @file result.h
 * @brief 结果类型定义，用于错误处理
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#ifndef DLOGCOVER_COMMON_RESULT_H
#define DLOGCOVER_COMMON_RESULT_H

#include <string>
#include <variant>
#include <type_traits>

namespace dlogcover {

/**
 * @brief 结果类模板
 * 
 * 用于表示操作的成功或失败结果，类似于Rust的Result类型
 * 
 * @tparam T 成功时的值类型
 * @tparam E 错误时的错误类型
 */
template<typename T, typename E>
class Result {
private:
    /**
     * @brief 错误信息结构
     */
    struct ErrorInfo {
        E code;
        std::string message;
    };

    std::variant<std::decay_t<T>, ErrorInfo> data_;  ///< 数据存储

public:
    /**
     * @brief 成功构造函数
     * @param value 成功值
     */
    explicit Result(T&& value) : data_(std::forward<T>(value)) {}

    /**
     * @brief 成功构造函数（左值）
     * @param value 成功值
     */
    explicit Result(const std::decay_t<T>& value) : data_(value) {}

    /**
     * @brief 错误构造函数
     * @param error 错误值
     * @param message 错误消息
     */
    Result(E error, const std::string& message) 
        : data_(ErrorInfo{error, message}) {}

    /**
     * @brief 检查是否成功
     * @return 如果成功返回true，否则返回false
     */
    bool isSuccess() const {
        return std::holds_alternative<std::decay_t<T>>(data_);
    }

    /**
     * @brief 检查是否失败
     * @return 如果失败返回true，否则返回false
     */
    bool isError() const {
        return std::holds_alternative<ErrorInfo>(data_);
    }

    /**
     * @brief 检查是否失败（别名方法）
     * @return 如果失败返回true，否则返回false
     */
    bool hasError() const {
        return isError();
    }

    /**
     * @brief explicit bool转换操作符
     * @return 如果成功返回true，否则返回false
     */
    explicit operator bool() const {
        return isSuccess();
    }

    /**
     * @brief 获取成功值
     * @return 成功值的引用
     * @throws std::bad_variant_access 如果不是成功状态
     */
    std::decay_t<T>& value() {
        return std::get<std::decay_t<T>>(data_);
    }

    /**
     * @brief 获取成功值（常量版本）
     * @return 成功值的常量引用
     * @throws std::bad_variant_access 如果不是成功状态
     */
    const std::decay_t<T>& value() const {
        return std::get<std::decay_t<T>>(data_);
    }

    /**
     * @brief 获取错误信息
     * @return 错误信息的引用
     * @throws std::bad_variant_access 如果不是错误状态
     */
    const ErrorInfo& errorInfo() const {
        return std::get<ErrorInfo>(data_);
    }

    /**
     * @brief 获取错误代码（兼容性方法）
     * @return 错误代码
     * @throws std::bad_variant_access 如果不是错误状态
     */
    E error() const {
        return std::get<ErrorInfo>(data_).code;
    }

    /**
     * @brief 获取错误消息
     * @return 错误消息
     * @throws std::bad_variant_access 如果不是错误状态
     */
    const std::string& errorMessage() const {
        return std::get<ErrorInfo>(data_).message;
    }
};

/**
 * @brief 创建成功结果的辅助函数（右值版本）
 * @tparam T 值类型
 * @tparam E 错误类型
 * @param value 成功值
 * @return 成功结果
 */
template<typename T, typename E>
Result<std::decay_t<T>, E> makeSuccess(T&& value) {
    return Result<std::decay_t<T>, E>(std::forward<T>(value));
}

/**
 * @brief 创建成功结果的辅助函数（左值版本）
 * @tparam T 值类型
 * @tparam E 错误类型
 * @param value 成功值
 * @return 成功结果
 */
template<typename T, typename E>
Result<T, E> makeSuccess(const T& value) {
    return Result<T, E>(value);
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