#include "../common/test_utils.h"

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

config::Config TestUtils::createTestConfig(const std::string& testDir) {
    config::Config config;

    // 设置扫描目录
    config.scan.directories = {testDir};

    // 设置文件类型
    config.scan.fileTypes = {".cpp", ".h", ".hpp", ".cc", ".c"};

    // 设置日志函数
    config.logFunctions.qt.enabled = true;
    config.logFunctions.qt.functions = {"qDebug", "qInfo", "qWarning", "qCritical", "qFatal"};

    // 设置分类日志函数
    config.logFunctions.custom.enabled = true;
    config.logFunctions.custom.functions["debug"] = {"LogDebug", "LOG_DEBUG", "log_debug"};
    config.logFunctions.custom.functions["info"] = {"LogInfo", "LOG_INFO", "log_info"};
    config.logFunctions.custom.functions["warning"] = {"LogWarning", "LOG_WARNING", "log_warning"};
    config.logFunctions.custom.functions["error"] = {"LogError", "LOG_ERROR", "log_error"};
    config.logFunctions.custom.functions["critical"] = {"LogCritical", "LOG_CRITICAL", "log_critical"};

    // 设置分析参数
    config.analysis.functionCoverage = true;
    config.analysis.branchCoverage = true;
    config.analysis.exceptionCoverage = true;
    config.analysis.keyPathCoverage = true;

    // 添加必要的编译参数
    config.scan.compilerArgs = {
        "-std=c++17", "-I/usr/include", "-I/usr/include/c++/8", "-I/usr/include/x86_64-linux-gnu/c++/8",
        "-I/usr/include/x86_64-linux-gnu", "-I/usr/local/include",
        // Qt头文件路径
        "-I/usr/include/x86_64-linux-gnu/qt5", "-I/usr/include/x86_64-linux-gnu/qt5/QtCore",
        "-I/usr/include/x86_64-linux-gnu/qt5/QtGui", "-I/usr/include/x86_64-linux-gnu/qt5/QtWidgets",
        // 系统定义
        "-D__GNUG__", "-D__linux__", "-D__x86_64__"};

    // 设置为Qt项目
    config.scan.isQtProject = true;

    return config;
}

std::unique_ptr<source_manager::SourceManager> TestUtils::createTestSourceManager(const config::Config& config) {
    return std::make_unique<source_manager::SourceManager>(config);
}

bool TestUtils::collectAndAnalyzeSource(source_manager::SourceManager& sourceManager,
                                        core::ast_analyzer::ASTAnalyzer& astAnalyzer) {
    // 收集源文件
    auto collectResult = sourceManager.collectSourceFiles();
    if (collectResult.hasError()) {
        return false;
    }

    // 分析所有文件
    auto analyzeResult = astAnalyzer.analyzeAll();
    if (analyzeResult.hasError()) {
        return false;
    }

    return true;
}

bool TestUtils::identifyLogs(core::ast_analyzer::ASTAnalyzer& astAnalyzer,
                             core::log_identifier::LogIdentifier& logIdentifier) {
    // 识别日志调用
    auto identifyResult = logIdentifier.identifyLogCalls();
    if (identifyResult.hasError()) {
        return false;
    }

    return true;
}

bool TestUtils::calculateCoverage(core::log_identifier::LogIdentifier& logIdentifier,
                                  core::ast_analyzer::ASTAnalyzer& astAnalyzer,
                                  core::coverage::CoverageCalculator& coverageCalculator) {
    // 计算覆盖率
    return coverageCalculator.calculate();
}

std::string TestUtils::createTestSourceFile(const std::string& dirPath, const std::string& filename,
                                            const std::string& content) {
    std::string filePath = dirPath + "/" + filename;
    std::ofstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("无法创建测试源文件: " + filePath);
    }

    file << content;
    file.close();

    return filePath;
}

}  // namespace test
}  // namespace dlogcover
