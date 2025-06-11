/**
 * @file ast_analyzer_parallel_test.cpp
 * @brief AST分析器并行处理单元测试
 * @copyright Copyright (c) 2023 DLogCover Team
 */

#include <gtest/gtest.h>
#include <dlogcover/core/ast_analyzer/ast_analyzer.h>
#include <dlogcover/config/config.h>
#include <dlogcover/source_manager/source_manager.h>
#include <dlogcover/config/config_manager.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace dlogcover {
namespace core {
namespace ast_analyzer {
namespace test {

class ASTAnalyzerParallelTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时测试目录
        testDir_ = std::filesystem::temp_directory_path() / "dlogcover_parallel_test";
        std::filesystem::create_directories(testDir_);
        
        // 创建测试配置
        config_.scan.directories = {testDir_.string()};
        config_.scan.file_extensions = {"*.cpp", "*.h"};
        config_.performance.enable_parallel_analysis = true;
        config_.performance.max_threads = 4;
        
        // 创建源文件管理器
        sourceManager_ = std::make_unique<source_manager::SourceManager>(config_);
        
        // 创建配置管理器
        configManager_ = std::make_unique<config::ConfigManager>();
    }
    
    void TearDown() override {
        // 清理测试文件
        std::filesystem::remove_all(testDir_);
    }
    
    void createTestFile(const std::string& filename, const std::string& content) {
        std::filesystem::path filePath = testDir_ / filename;
        std::ofstream file(filePath);
        file << content;
        file.close();
    }
    
    std::filesystem::path testDir_;
    config::Config config_;
    std::unique_ptr<source_manager::SourceManager> sourceManager_;
    std::unique_ptr<config::ConfigManager> configManager_;
};

TEST_F(ASTAnalyzerParallelTest, ParallelAnalysisProcessesAllFiles) {
    // 创建多个测试文件
    createTestFile("test1.cpp", R"(
        #include <iostream>
        void function1() {
            std::cout << "test1" << std::endl;
        }
    )");
    
    createTestFile("test2.cpp", R"(
        #include <iostream>
        void function2() {
            std::cout << "test2" << std::endl;
        }
    )");
    
    createTestFile("test3.cpp", R"(
        #include <iostream>
        void function3() {
            std::cout << "test3" << std::endl;
        }
    )");
    
    // 收集源文件
    sourceManager_->collectSourceFiles();
    
    // 创建AST分析器
    ASTAnalyzer analyzer(config_, *sourceManager_, *configManager_);
    analyzer.setParallelMode(true, 4);
    
    // 执行并行分析
    auto result = analyzer.analyzeAllParallel();
    
    // 验证分析成功
    EXPECT_TRUE(result.isSuccess()) << "并行分析应该成功";
    
    // 验证所有文件都被处理
    const auto& astNodes = analyzer.getAllASTNodeInfo();
    EXPECT_EQ(astNodes.size(), 3) << "应该处理3个文件";
    
    // 验证每个文件都有对应的AST节点信息
    bool found1 = false, found2 = false, found3 = false;
    for (const auto& pair : astNodes) {
        if (pair.first.find("test1.cpp") != std::string::npos) found1 = true;
        if (pair.first.find("test2.cpp") != std::string::npos) found2 = true;
        if (pair.first.find("test3.cpp") != std::string::npos) found3 = true;
    }
    
    EXPECT_TRUE(found1) << "test1.cpp应该被处理";
    EXPECT_TRUE(found2) << "test2.cpp应该被处理";
    EXPECT_TRUE(found3) << "test3.cpp应该被处理";
}

TEST_F(ASTAnalyzerParallelTest, ParallelAnalysisConsistentWithSequential) {
    // 创建测试文件
    createTestFile("consistency_test.cpp", R"(
        #include <iostream>
        #include <vector>
        
        class TestClass {
        public:
            void method1() {
                std::cout << "method1" << std::endl;
            }
            
            void method2() {
                std::vector<int> vec = {1, 2, 3};
                for (int i : vec) {
                    std::cout << i << std::endl;
                }
            }
        };
        
        int main() {
            TestClass obj;
            obj.method1();
            obj.method2();
            return 0;
        }
         )");
     
     sourceManager_->collectSourceFiles();
     
     // 顺序分析
    ASTAnalyzer sequentialAnalyzer(config_, *sourceManager_, *configManager_);
    sequentialAnalyzer.setParallelMode(false);
    auto sequentialResult = sequentialAnalyzer.analyzeAll();
    
    // 并行分析
    ASTAnalyzer parallelAnalyzer(config_, *sourceManager_, *configManager_);
    parallelAnalyzer.setParallelMode(true, 2);
    auto parallelResult = parallelAnalyzer.analyzeAllParallel();
    
    // 验证两种方式都成功
    EXPECT_TRUE(sequentialResult.isSuccess()) << "顺序分析应该成功";
    EXPECT_TRUE(parallelResult.isSuccess()) << "并行分析应该成功";
    
    // 验证结果一致性（文件数量应该相同）
    const auto& sequentialNodes = sequentialAnalyzer.getAllASTNodeInfo();
    const auto& parallelNodes = parallelAnalyzer.getAllASTNodeInfo();
    
    EXPECT_EQ(sequentialNodes.size(), parallelNodes.size()) 
        << "顺序分析和并行分析应该处理相同数量的文件";
}

TEST_F(ASTAnalyzerParallelTest, ParallelAnalysisThreadSafety) {
    // 创建多个文件进行压力测试
    for (int i = 0; i < 10; ++i) {
        std::string filename = "thread_test_" + std::to_string(i) + ".cpp";
        std::string content = R"(
            #include <iostream>
            void function)" + std::to_string(i) + R"(() {
                std::cout << "function)" + std::to_string(i) + R"(" << std::endl;
                // 添加一些复杂的代码结构
                for (int j = 0; j < 10; ++j) {
                    if (j % 2 == 0) {
                        std::cout << "even: " << j << std::endl;
                    } else {
                        std::cout << "odd: " << j << std::endl;
                    }
                }
            }
        )";
        createTestFile(filename, content);
         }
     
     sourceManager_->collectSourceFiles();
     
     // 多次运行并行分析，检查线程安全性
    for (int run = 0; run < 3; ++run) {
        ASTAnalyzer analyzer(config_, *sourceManager_, *configManager_);
        analyzer.setParallelMode(true, 8);
        
        auto result = analyzer.analyzeAllParallel();
        
        EXPECT_TRUE(result.isSuccess()) << "第" << (run + 1) << "次并行分析应该成功";
        
        const auto& astNodes = analyzer.getAllASTNodeInfo();
        EXPECT_EQ(astNodes.size(), 10) << "第" << (run + 1) << "次分析应该处理10个文件";
    }
}

TEST_F(ASTAnalyzerParallelTest, ParallelAnalysisPerformance) {
    // 创建足够多的文件来测试性能差异
    for (int i = 0; i < 20; ++i) {
        std::string filename = "perf_test_" + std::to_string(i) + ".cpp";
        std::string content = R"(
            #include <iostream>
            #include <vector>
            #include <string>
            #include <algorithm>
            
            class PerfTestClass)" + std::to_string(i) + R"( {
            private:
                std::vector<std::string> data_;
                
            public:
                void processData() {
                    for (size_t i = 0; i < 100; ++i) {
                        data_.push_back("item_" + std::to_string(i));
                    }
                    
                    std::sort(data_.begin(), data_.end());
                    
                    for (const auto& item : data_) {
                        std::cout << item << std::endl;
                    }
                }
                
                void complexMethod() {
                    try {
                        processData();
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                    }
                }
            };
        )";
        createTestFile(filename, content);
         }
     
     sourceManager_->collectSourceFiles();
     
     // 测量顺序分析时间
    auto start = std::chrono::high_resolution_clock::now();
    ASTAnalyzer sequentialAnalyzer(config_, *sourceManager_, *configManager_);
    sequentialAnalyzer.setParallelMode(false);
    auto sequentialResult = sequentialAnalyzer.analyzeAll();
    auto sequentialTime = std::chrono::high_resolution_clock::now() - start;
    
    // 测量并行分析时间
    start = std::chrono::high_resolution_clock::now();
    ASTAnalyzer parallelAnalyzer(config_, *sourceManager_, *configManager_);
    parallelAnalyzer.setParallelMode(true, 4);
    auto parallelResult = parallelAnalyzer.analyzeAllParallel();
    auto parallelTime = std::chrono::high_resolution_clock::now() - start;
    
    // 验证两种方式都成功
    EXPECT_TRUE(sequentialResult.isSuccess()) << "顺序分析应该成功";
    EXPECT_TRUE(parallelResult.isSuccess()) << "并行分析应该成功";
    
    // 输出性能比较信息（用于调试）
    auto sequentialMs = std::chrono::duration_cast<std::chrono::milliseconds>(sequentialTime).count();
    auto parallelMs = std::chrono::duration_cast<std::chrono::milliseconds>(parallelTime).count();
    
    std::cout << "顺序分析时间: " << sequentialMs << "ms" << std::endl;
    std::cout << "并行分析时间: " << parallelMs << "ms" << std::endl;
    
    // 在多核系统上，并行分析通常应该更快（但这个测试可能因系统而异）
    // 这里主要验证功能正确性，性能提升是额外的好处
}

} // namespace test
} // namespace ast_analyzer
} // namespace core
} // namespace dlogcover 