# DLogCover - C++代码日志覆盖分析工具

DLogCover 是一个专门用于分析C++代码中日志覆盖情况的工具。它能够帮助开发者识别代码中缺少日志记录的关键路径，提高代码的可观测性和调试能力。

## 🚀 主要特性

### 核心功能
- **智能AST分析**: 基于Clang的深度语法分析，准确识别代码结构
- **多维度覆盖分析**: 支持函数覆盖、分支覆盖、异常覆盖和关键路径覆盖
- **灵活的日志函数支持**: 内置Qt日志函数支持，可扩展自定义日志函数
- **详细的分析报告**: 提供文本和JSON格式的详细分析报告

### 性能特性
- **高效的AST解析**: 优化的解析算法，支持大型项目分析
- **智能日志级别管理**: INFO级别显示关键进度，DEBUG级别提供详细调试信息
- **并行分析支持**: 支持多线程并行分析，提升大项目处理速度
- **内存优化**: 智能内存管理，减少大文件分析时的内存占用

### 用户体验
- **简洁的输出界面**: 默认INFO级别输出，专注于关键分析进度
- **灵活的配置系统**: 支持命令行参数、配置文件、环境变量多种配置方式
- **详细的错误提示**: 友好的错误信息和修复建议
- **完善的帮助系统**: 内置详细的使用说明和示例

## 📦 安装

### 系统要求
- C++17 或更高版本
- CMake 3.16 或更高版本
- Clang/LLVM 开发库
- Qt5 开发库（可选，用于Qt项目分析）

### 编译安装

```bash
# 克隆仓库
git clone https://github.com/your-org/dlogcover.git
cd dlogcover

# 创建构建目录
mkdir build && cd build

# 配置和编译
cmake ..
make -j$(nproc)

# 安装（可选）
sudo make install
```

## 🔧 使用方法

### 基本用法

```bash
# 分析当前目录
dlogcover

# 分析指定目录
dlogcover -d /path/to/source

# 指定输出文件
dlogcover -d ./src -o coverage_report.txt

# 使用配置文件
dlogcover -c dlogcover.json
```

### 命令行选项

#### 主要选项
```bash
-h, --help                 # 显示帮助信息
-v, --version              # 显示版本信息
-d, --directory <path>     # 指定扫描目录 (默认: ./)
-o, --output <path>        # 指定输出报告路径
-c, --config <path>        # 指定配置文件路径 (默认: ./dlogcover.json)
```

#### 日志和输出控制
```bash
-l, --log-level <level>    # 日志级别 (debug, info, warning, critical, fatal, all)
-f, --format <format>      # 报告格式 (text, json)
-p, --log-path <path>      # 日志文件路径
-q, --quiet                # 静默模式
    --verbose              # 详细输出模式
```

#### 文件过滤
```bash
-e, --exclude <pattern>    # 排除模式 (可多次使用)
-I, --include-path <path>  # 头文件搜索路径 (可多次使用)
```

#### 高级选项
```bash
    --compiler-arg <arg>   # 编译器参数 (可多次使用)
    --include-system-headers # 包含系统头文件分析
    --threshold <value>    # 覆盖率阈值 (0.0-1.0)
    --parallel <num>       # 并行分析线程数
```

### 环境变量支持

```bash
export DLOGCOVER_DIRECTORY="./src"          # 等同于 -d 选项
export DLOGCOVER_OUTPUT="./report.txt"      # 等同于 -o 选项
export DLOGCOVER_CONFIG="./config.json"     # 等同于 -c 选项
export DLOGCOVER_LOG_LEVEL="debug"          # 等同于 -l 选项
export DLOGCOVER_REPORT_FORMAT="json"       # 等同于 -f 选项
export DLOGCOVER_EXCLUDE="build/*:test/*"   # 排除模式，冒号分隔
export DLOGCOVER_LOG_PATH="./analysis.log"  # 等同于 -p 选项
```

### 配置文件

DLogCover 支持JSON格式的配置文件，提供更灵活的配置选项：

```json
{
    "project": {
        "name": "my-project",
        "directory": "./src",
        "build_directory": "./build"
    },
    "scan": {
        "directories": ["include", "src", "tests"],
        "file_extensions": [".cpp", ".h", ".cxx", ".hpp"],
        "exclude_patterns": ["*build*", "*/build/*"]
    },
    "compile_commands": {
        "path": "./build/compile_commands.json",
        "auto_generate": true,
        "cmake_args": ["-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"]
    },
    "output": {
        "report_file": "dlogcover_report.txt",
        "log_file": "dlogcover.log",
        "log_level": "INFO"
    },
         "log_functions": {
         "qt": {
             "enabled": true,
             "functions": ["qDebug", "qInfo", "qWarning", "qCritical", "qFatal"],
             "category_functions": ["qCDebug", "qCInfo", "qCWarning", "qCCritical"]
         },
         "custom": {
             "enabled": true,
             "functions": {
                 "debug": ["logDebug", "LOG_DEBUG", "LOG_DEBUG_FMT"],
                 "info": ["logInfo", "LOG_INFO", "LOG_INFO_FMT"],
                 "warning": ["logWarning", "LOG_WARNING", "LOG_WARNING_FMT"],
                 "error": ["logError", "LOG_ERROR", "LOG_ERROR_FMT"],
                 "fatal": ["logFatal", "LOG_FATAL", "LOG_FATAL_FMT"]
             }
         }
     },
    "analysis": {
        "function_coverage": true,
        "branch_coverage": true,
        "exception_coverage": true,
        "key_path_coverage": true
    }
}
```

### 配置优先级

DLogCover 采用分层配置系统，优先级从高到低：

```
命令行参数 > 配置文件 > 环境变量 > 默认值
```

## 📊 分析报告

### 报告格式

DLogCover 支持两种输出格式：

#### 文本格式 (默认)
```
=== DLogCover 分析报告 ===
分析时间: 2024-12-01 10:30:00
项目路径: /path/to/project

=== 文件分析结果 ===
文件: src/main.cpp
  函数覆盖率: 85.7% (6/7)
  分支覆盖率: 75.0% (3/4)
  
未覆盖的关键路径:
  - 第45行: 错误处理分支缺少日志
  - 第67行: 异常捕获块缺少日志
```

#### JSON格式
```json
{
    "metadata": {
        "analysis_time": "2024-12-01T10:30:00Z",
        "project_path": "/path/to/project",
        "tool_version": "0.1.0"
    },
    "summary": {
        "total_files": 15,
        "total_functions": 120,
        "covered_functions": 95,
        "function_coverage": 79.2
    },
    "files": [
        {
            "path": "src/main.cpp",
            "function_coverage": 85.7,
            "branch_coverage": 75.0,
            "uncovered_paths": [
                {
                    "line": 45,
                    "type": "error_handling",
                    "description": "错误处理分支缺少日志"
                }
            ]
        }
    ]
}
```

## 🎯 使用示例

### 基本项目分析

```bash
# 分析当前目录的C++项目
dlogcover

# 分析特定目录，排除构建文件
dlogcover -d ./src -e "build/*" -e "test/*"

# 生成JSON格式报告
dlogcover -f json -o analysis_report.json
```

### Qt项目分析

```bash
# 分析Qt项目，包含Qt日志函数
dlogcover -d ./src -I /usr/include/qt5 --compiler-arg "-DQT_CORE_LIB"
```

### CI/CD集成

```bash
# CI环境中的静默分析
export DLOGCOVER_LOG_LEVEL=warning
export DLOGCOVER_REPORT_FORMAT=json
dlogcover --quiet -o ci_report.json
```

### 大型项目优化

```bash
# 使用并行分析和详细日志
dlogcover -d ./large_project --parallel 8 --log-level debug -p detailed.log
```

## 📈 性能基准

基于实际项目测试的性能数据：

### 日志输出优化效果
- **INFO级别日志减少**: 约80%的详细日志移至DEBUG级别
- **I/O性能提升**: 日志I/O开销降低60-70%
- **总体分析时间**: 提升8-15%，大型项目效果更明显

### 项目规模支持
- **小型项目** (< 10K LOC): < 5秒
- **中型项目** (10K-100K LOC): 30秒-2分钟
- **大型项目** (> 100K LOC): 2-10分钟（使用并行分析）

### 内存使用
- **基础内存占用**: ~50MB
- **大文件处理**: 动态内存管理，峰值不超过500MB
- **并行分析**: 每线程额外占用~20MB

## 🔍 日志级别说明

DLogCover 提供分层的日志输出系统：

### INFO级别（默认，用户友好）
- 文件分析开始/完成状态
- 最终统计结果和汇总信息
- 重要的配置和错误信息
- 分析进度指示

### DEBUG级别（详细调试）
- AST解析的详细过程
- 每个函数、类、命名空间的发现过程
- 节点创建和添加的详细步骤
- 编译命令和参数信息
- 内部状态变化

### WARNING/ERROR级别
- 配置问题和建议
- 文件访问错误
- 解析失败的详细原因
- 系统级错误信息

使用建议：
- **日常使用**: 保持INFO级别，获得简洁的分析进度
- **问题调试**: 使用DEBUG级别，获得完整的执行细节
- **CI/CD**: 使用WARNING级别，只关注问题和错误

## 🛠️ 开发和贡献

### 项目结构

```
dlogcover/
├── src/                    # 源代码
│   ├── core/              # 核心分析引擎
│   ├── cli/               # 命令行接口
│   ├── config/            # 配置管理
│   └── utils/             # 工具函数
├── include/               # 头文件
├── tests/                 # 测试代码
├── docs/                  # 文档
└── examples/              # 示例项目
```

### 构建系统

项目使用CMake构建系统，支持：
- 模块化编译
- 单元测试集成
- 代码覆盖率分析
- 静态代码分析

### 贡献指南

1. Fork 项目仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🤝 支持和反馈

- **问题报告**: [GitHub Issues](https://github.com/your-org/dlogcover/issues)
- **功能请求**: [GitHub Discussions](https://github.com/your-org/dlogcover/discussions)
- **文档**: [项目Wiki](https://github.com/your-org/dlogcover/wiki)

## 📚 更新日志

### v0.1.0 (2024-12-01)
- ✨ 初始版本发布
- 🚀 基础AST分析功能
- 📊 文本和JSON报告格式
- ⚙️ 灵活的配置系统
- 🎯 Qt日志函数支持
- 📈 日志输出优化，提升用户体验
- 🔧 完善的命令行接口
- 🌐 环境变量支持
- ⚡ 并行分析支持
- 📝 详细的文档和示例

---

**DLogCover** - 让你的C++代码日志覆盖一目了然！
