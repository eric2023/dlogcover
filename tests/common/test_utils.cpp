#include "test_utils.h"

#include <chrono>
#include <filesystem>
#include <future>
#include <random>
#include <thread>

namespace dlogcover {
namespace test {

std::string TestUtils::generateUniqueDirName(const std::string& prefix) {
    // 使用随机数生成器创建唯一名称
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999);

    return prefix + std::to_string(dis(gen));
}

std::string TestUtils::createTestTempDir(const std::string& prefix, std::chrono::seconds timeout) {
    auto start_time = std::chrono::steady_clock::now();
    std::error_code ec;

    while (true) {
        // 检查是否超时
        auto current_time = std::chrono::steady_clock::now();
        if (current_time - start_time > timeout) {
            throw std::runtime_error("创建临时目录超时");
        }

        // 在系统临时目录下创建测试目录
        auto temp_path = std::filesystem::temp_directory_path() / generateUniqueDirName(prefix);

        // 尝试创建目录
        if (std::filesystem::create_directories(temp_path, ec)) {
            return temp_path.string();
        }

        // 如果目录已存在，等待一小段时间后重试
        if (ec) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
    }
}

bool TestUtils::cleanupTestTempDir(const std::string& path, std::chrono::seconds timeout) {
    auto cleanup_task = std::async(std::launch::async, [&]() {
        std::error_code ec;
        return std::filesystem::remove_all(path, ec);
    });

    // 等待清理完成或超时
    auto status = cleanup_task.wait_for(timeout);
    if (status == std::future_status::timeout) {
        return false;
    }

    return cleanup_task.get() > 0;
}

config::Config TestUtils::createTestConfig() {
    config::Config config;
    // 设置基本的测试配置
    config.scan.isQtProject = true;
    config.scan.includePathsStr = "/usr/include";
    config.scan.compilerArgs = {"-std=c++17"};

    return config;
}

source_manager::SourceManager TestUtils::createTestSourceManager() {
    return source_manager::SourceManager();
}

}  // namespace test
}  // namespace dlogcover
