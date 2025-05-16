#pragma once

#include <dlogcover/config/config.h>
#include <dlogcover/source_manager/source_manager.h>

#include <chrono>
#include <string>
#include <system_error>

namespace dlogcover {
namespace test {

class TestUtils {
public:
    // 创建临时测试目录
    static std::string createTestTempDir(const std::string& prefix = "test_",
                                         std::chrono::seconds timeout = std::chrono::seconds(5));

    // 清理临时测试目录
    static bool cleanupTestTempDir(const std::string& path, std::chrono::seconds timeout = std::chrono::seconds(5));

    // 创建测试配置
    static config::Config createTestConfig();

    // 创建测试源码管理器
    static source_manager::SourceManager createTestSourceManager();

private:
    // 生成唯一的临时目录名
    static std::string generateUniqueDirName(const std::string& prefix);
};

}  // namespace test
}  // namespace dlogcover
