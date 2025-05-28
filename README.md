# DLogCover - C++日志覆盖分析工具

DLogCover是一个基于Clang/LLVM的C++日志覆盖分析工具，用于分析C++项目中的日志记录覆盖情况。

## 项目概述

DLogCover通过分析C++源代码的抽象语法树(AST)，识别代码中的函数、分支、循环、异常处理等结构，并检测这些结构中是否包含适当的日志记录。工具支持Qt日志系统和自定义日志函数，能够生成详细的覆盖率报告。

## 最新更新 (2024)

### 项目头文件架构 (v0.1.0)
项目包含29个头文件，按功能模块组织如下：

#### CLI模块 (5个文件)
- `include/dlogcover/cli/command_line_parser.h` - 命令行解析器
- `include/dlogcover/cli/config_constants.h` - 配置常量定义
- `include/dlogcover/cli/config_validator.h` - 配置验证器
- `include/dlogcover/cli/error_types.h` - 错误类型枚举
- `include/dlogcover/cli/options.h` - 命令行选项定义

#### 配置模块 (2个文件)
- `include/dlogcover/config/config.h` - 配置结构定义
- `include/dlogcover/config/config_manager.h` - 配置管理器

#### 工具模块 (3个文件)
- `include/dlogcover/utils/file_utils.h` - 文件操作工具
- `include/dlogcover/utils/log_utils.h` - 日志工具函数
- `include/dlogcover/utils/string_utils.h` - 字符串处理工具

#### 核心分析模块 (10个文件)
##### AST分析器 (6个文件)
- `include/dlogcover/core/ast_analyzer/ast_analyzer.h` - AST分析器主要接口
- `include/dlogcover/core/ast_analyzer/ast_expression_analyzer.h` - 表达式分析器
- `include/dlogcover/core/ast_analyzer/ast_function_analyzer.h` - 函数分析器
- `include/dlogcover/core/ast_analyzer/ast_node_analyzer.h` - AST节点分析器
- `include/dlogcover/core/ast_analyzer/ast_statement_analyzer.h` - 语句分析器
- `include/dlogcover/core/ast_analyzer/ast_types.h` - AST分析器类型定义

##### 日志识别器 (2个文件)
- `include/dlogcover/core/log_identifier/log_identifier.h` - 日志识别器
- `include/dlogcover/core/log_identifier/log_types.h` - 日志类型定义

##### 覆盖率计算器 (2个文件)
- `include/dlogcover/core/coverage/coverage_calculator.h` - 覆盖率计算器
- `include/dlogcover/core/coverage/coverage_stats.h` - 覆盖率统计信息

#### 源文件管理模块 (1个文件)
- `include/dlogcover/source_manager/source_manager.h` - 源文件管理器

#### 报告生成模块 (7个文件)
- `include/dlogcover/reporter/ireporter_strategy.h` - 报告策略接口
- `include/dlogcover/reporter/json_reporter_strategy.h` - JSON报告策略
- `include/dlogcover/reporter/reporter.h` - 报告生成器
- `include/dlogcover/reporter/reporter_factory.h` - 报告工厂
- `include/dlogcover/reporter/reporter_types.h` - 报告类型定义
- `include/dlogcover/reporter/text_reporter_strategy.h` - 文本报告策略

#### 公共模块 (1个文件)
- `include/dlogcover/common/result.h` - 结果类型定义

### 头文件特点
- 严格遵循项目代码规范
- 完整的文档注释
- 合理的命名空间组织 (`dlogcover::`)
- 清晰的模块依赖关系
- 支持现代C++17特性

### 默认配置文件
项目提供标准配置文件 `dlogcover.json`：

```json
{
    "scan": {
        "directories": [
            "./"
        ],
        "excludes": [
            "build/",
            "test/",
            "third_party/",
            ".git/",
            ".vscode/",
            ".cursor/",
            ".specstory/"
        ],
        "file_types": [
            ".cpp",
            ".cc",
            ".cxx",
            ".h",
            ".hpp",
            ".hxx"
        ],
        "include_paths": "./include:./src",
        "is_qt_project": false,
        "compiler_args": [
            "-Wno-everything",
            "-xc++",
            "-ferror-limit=0",
            "-fsyntax-only",
            "-std=c++17"
        ]
    },
    "log_functions": {
        "qt": {
            "enabled": true,
            "functions": [
                "qDebug",
                "qInfo",
                "qWarning",
                "qCritical",
                "qFatal"
            ],
            "category_functions": [
                "qCDebug",
                "qCInfo",
                "qCWarning",
                "qCCritical"
            ]
        },
        "custom": {
            "enabled": true,
            "functions": {
                "debug": [
                    "fmDebug",
                    "LOG_DEBUG",
                    "LOG_DEBUG_FMT"
                ],
                "info": [
                    "fmInfo",
                    "LOG_INFO",
                    "LOG_INFO_FMT"
                ],
                "warning": [
                    "fmWarning",
                    "LOG_WARNING",
                    "LOG_WARNING_FMT"
                ],
                "critical": [
                    "fmCritical"
                ],
                "fatal": [
                    "LOG_ERROR",
                    "LOG_ERROR_FMT",
                    "LOG_FATAL",
                    "LOG_FATAL_FMT"
                ]
            }
        }
    },
    "analysis": {
        "function_coverage": true,
        "branch_coverage": true,
        "exception_coverage": true,
        "key_path_coverage": true
    },
    "report": {
        "format": "text",
        "timestamp_format": "YYYYMMDD_HHMMSS"
    }
}
```

### 配置文件说明
- **扫描配置**: 控制扫描范围、排除文件和编译参数
- **日志函数**: 定义Qt和自定义日志函数识别规则
- **分析选项**: 启用/禁用不同类型的覆盖率分析
- **报告格式**: 控制输出格式和时间戳样式

## 主要功能

### 1. 多层次覆盖分析
- **函数覆盖率**: 分析函数是否包含日志记录
- **分支覆盖率**: 检测if/else、switch等分支结构的日志覆盖
- **循环覆盖率**: 分析for、while等循环结构的日志记录
- **异常处理覆盖率**: 检测try-catch块的日志覆盖情况
- **关键路径覆盖率**: 识别和分析关键执行路径的日志覆盖

### 2. 灵活的日志系统支持
- **Qt日志系统**: 支持qDebug、qInfo、qWarning、qCritical、qFatal等
- **Qt分类日志**: 支持qCDebug、qCInfo等分类日志函数
- **自定义日志**: 通过配置文件支持任意自定义日志函数
- **日志级别过滤**: 支持按日志级别进行过滤分析

### 3. 多格式报告输出
- **文本格式**: 易读的文本报告
- **JSON格式**: 结构化数据，便于集成其他工具
- **详细统计**: 提供文件级和项目级的详细统计信息

### 4. 灵活的配置系统
- **JSON配置文件**: 支持复杂的配置选项
- **命令行参数**: 支持常用参数的命令行覆盖
- **排除模式**: 支持正则表达式排除特定文件或目录

## 技术架构

### 核心组件
1. **AST分析器** (`core/ast_analyzer/`): 基于Clang解析C++源代码
   - `ast_analyzer.h` - 主要分析接口
   - `ast_function_analyzer.h` - 函数级分析
   - `ast_statement_analyzer.h` - 语句级分析
   - `ast_expression_analyzer.h` - 表达式分析
   - `ast_node_analyzer.h` - 通用节点分析
   - `ast_types.h` - AST类型定义

2. **日志识别器** (`core/log_identifier/`): 识别代码中的日志调用
   - `log_identifier.h` - 日志识别核心逻辑
   - `log_types.h` - 日志类型定义

3. **覆盖率计算器** (`core/coverage/`): 计算各种类型的覆盖率
   - `coverage_calculator.h` - 覆盖率计算逻辑
   - `coverage_stats.h` - 覆盖率统计数据结构

4. **源文件管理器** (`source_manager/`): 管理源文件的收集和处理
   - `source_manager.h` - 源文件扫描和管理

5. **报告生成器** (`reporter/`): 生成各种格式的覆盖率报告
   - `reporter.h` - 报告生成主接口
   - `ireporter_strategy.h` - 报告策略抽象接口
   - `text_reporter_strategy.h` - 文本格式报告实现
   - `json_reporter_strategy.h` - JSON格式报告实现
   - `reporter_factory.h` - 报告器工厂
   - `reporter_types.h` - 报告相关类型定义

6. **配置管理** (`config/`): 处理配置文件和参数
   - `config.h` - 配置数据结构
   - `config_manager.h` - 配置管理器

7. **命令行接口** (`cli/`): 处理命令行参数和验证
   - `command_line_parser.h` - 命令行解析
   - `options.h` - 选项定义
   - `config_validator.h` - 配置验证
   - `config_constants.h` - 配置常量
   - `error_types.h` - 错误类型

8. **工具模块** (`utils/`): 提供通用工具函数
   - `file_utils.h` - 文件操作工具
   - `log_utils.h` - 日志工具
   - `string_utils.h` - 字符串处理工具

### 支持的编译器
- GCC 7.0+
- Clang 6.0+
- 需要C++17支持

## 安装和构建

### 系统依赖项
项目需要以下系统依赖：

#### 核心依赖
- **LLVM/Clang开发库**: 用于AST分析
- **nlohmann/json**: JSON配置文件处理
- **GoogleTest**: 单元测试框架（必需）
- **lcov**: 代码覆盖率报告生成

#### Ubuntu/Debian 安装命令
```bash
# 基础构建工具
sudo apt-get update
sudo apt-get install build-essential cmake

# LLVM/Clang开发库
sudo apt-get install libclang-dev llvm-dev clang

# Clang库组件 (项目需要以下库)
# clangTooling, clangBasic, clangAST, clangASTMatchers
# clangFrontend, clangLex, clangParse, clangSema
# clangAnalysis, clangDriver, clangEdit, clangRewrite, clangSerialization

# JSON库
sudo apt-get install nlohmann-json3-dev

# 测试和覆盖率工具
sudo apt-get install libgtest-dev lcov

# 可选：安装完整的clang工具链
sudo apt-get install clang-tools
```

#### CentOS/RHEL 安装命令
```bash
# 基础构建工具
sudo yum groupinstall "Development Tools"
sudo yum install cmake

# LLVM/Clang开发库
sudo yum install clang-devel llvm-devel

# JSON库
sudo yum install nlohmann-json-devel

# 测试工具
sudo yum install gtest-devel lcov
```

### 构建步骤

#### 方法1: 使用CMake手动构建
```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DENABLE_COVERAGE=ON ..

# 编译项目
cmake --build . -- -j$(nproc)

# 运行测试
ctest --verbose

# 安装(可选)
sudo cmake --install .
```

#### 方法2: 使用构建脚本(推荐)
项目提供了便捷的构建脚本 `build.sh`：

```bash
# 显示帮助信息
./build.sh --help

# 基本构建
./build.sh

# 构建并运行测试
./build.sh --test

# 构建发布版本
./build.sh --release

# 执行完整流程(编译→测试→覆盖率统计)
./build.sh --full-process

# 清理并重新构建
./build.sh --clean --test

# 安装到指定路径
./build.sh --install --prefix=/opt/dlogcover
```

#### 构建脚本选项说明
| 选项 | 描述 |
|------|------|
| `-h, --help` | 显示帮助信息 |
| `-c, --clean` | 清理构建目录 |
| `-d, --debug` | 构建Debug版本(默认) |
| `-r, --release` | 构建Release版本 |
| `-t, --test` | 构建并运行测试 |
| `-i, --install` | 安装到系统 |
| `-f, --full-process` | 执行完整流程：编译→测试→覆盖率统计 |
| `--prefix=<path>` | 安装路径前缀(默认: /usr/local) |

### 构建输出
成功构建后将生成：
- `build/bin/dlogcover` - 主程序可执行文件
- `build/lib/` - 静态库文件(如果有)
- `build/coverage_report/` - 覆盖率报告(使用--full-process时)

## 使用方法

### 基本用法
```bash
# 分析当前目录
./dlogcover

# 分析指定目录
./dlogcover -d /path/to/project

# 使用配置文件
./dlogcover -c dlogcover.json

# 生成JSON格式报告
./dlogcover -d /path/to/project -o report.json -f json

# 使用完整流程分析
./build.sh --full-process  # 包含编译、测试、覆盖率分析
```

### 配置文件详解

#### 扫描配置 (`scan`)
- `directories`: 扫描的目录列表
- `excludes`: 排除的目录模式列表  
- `file_types`: 支持的文件扩展名
- `include_paths`: 头文件搜索路径 (冒号分隔)
- `is_qt_project`: 是否为Qt项目
- `compiler_args`: 编译器参数列表

#### 日志函数配置 (`log_functions`)
- `qt.enabled`: 是否启用Qt日志检测
- `qt.functions`: Qt标准日志函数列表
- `qt.category_functions`: Qt分类日志函数列表
- `custom.enabled`: 是否启用自定义日志检测
- `custom.functions`: 按级别分组的自定义日志函数

#### 分析配置 (`analysis`)
- `function_coverage`: 启用函数覆盖率分析
- `branch_coverage`: 启用分支覆盖率分析  
- `exception_coverage`: 启用异常处理覆盖率分析
- `key_path_coverage`: 启用关键路径覆盖率分析

#### 报告配置 (`report`)
- `format`: 报告格式 (`text` 或 `json`)
- `timestamp_format`: 时间戳格式

## 命令行选项

### 主要选项
| 选项 | 长选项 | 描述 | 默认值 | 示例 |
|------|--------|------|--------|------|
| `-d` | `--directory` | 扫描目录路径 | 当前目录 | `-d /path/to/project` |
| `-o` | `--output` | 输出文件路径 | 标准输出 | `-o report.json` |
| `-c` | `--config` | 配置文件路径 | 无 | `-c config.json` |
| `-f` | `--format` | 报告格式 | text | `-f json` |
| `-l` | `--log-level` | 日志级别过滤 | all | `-l warning` |
| `-e` | `--exclude` | 排除模式 | 无 | `-e "test/*"` |
| `-I` | `--include-path` | 头文件搜索路径 | 无 | `-I ./include` |
| `-q` | `--quiet` | 静默模式 | false | `-q` |
| `-v` | `--verbose` | 详细输出 | false | `-v` |
| `-h` | `--help` | 显示帮助信息 | - | `-h` |
| `--version` | | 显示版本信息 | - | `--version` |

### 高级选项
| 选项 | 描述 | 示例 |
|------|------|------|
| `--compiler-arg` | 传递给编译器的参数 | `--compiler-arg "-std=c++17"` |
| `--include-system-headers` | 包含系统头文件分析 | `--include-system-headers` |
| `--threshold` | 覆盖率阈值设置 | `--threshold 80` |
| `--parallel` | 并行分析线程数 | `--parallel 4` |

### 使用示例
```bash
# 基本分析
./dlogcover -d ./src -f json -o coverage_report.json

# 使用自定义配置
./dlogcover -c my_config.json -v

# 过滤特定日志级别
./dlogcover -l error -f text

# 排除测试文件
./dlogcover -e "test/" -e "third_party/" -d ./src

# 并行分析大型项目
./dlogcover -d ./large_project --parallel 8 --quiet
```

## 报告示例

### 文本格式报告
```
DLogCover 覆盖率报告
====================
项目: MyProject
时间: 2024-01-15 14:30:25

总体统计:
- 总文件数: 25
- 有覆盖的文件数: 20
- 总函数数: 150
- 有日志的函数数: 120
- 函数覆盖率: 80.00%
- 分支覆盖率: 75.50%
- 异常处理覆盖率: 90.00%
- 关键路径覆盖率: 85.00%
- 总体覆盖率: 82.63%

文件详情:
src/main.cpp: 95.00% (19/20 函数有日志)
src/utils.cpp: 70.00% (14/20 函数有日志)
...
```

### JSON格式报告
```json
{
    "projectName": "MyProject",
    "timestamp": "2024-01-15T14:30:25",
    "overallCoverageRatio": 0.8263,
    "totalFiles": 25,
    "coveredFiles": 20,
    "totalFunctions": 150,
    "coveredFunctions": 120,
    "functionCoverageRatio": 0.8000,
    "files": [
        {
            "filePath": "src/main.cpp",
            "functionCoverageRatio": 0.95,
            "functions": [...]
        }
    ]
}
```

## 开发和贡献

### 代码规范
- 遵循Google C++代码规范
- 使用现代C++特性(C++17)
- 完整的单元测试覆盖
- 详细的代码注释

### 测试
```bash
# 运行单元测试
make test

# 运行集成测试
./tests/integration_test.sh
```

### 贡献指南
1. Fork项目
2. 创建特性分支
3. 提交更改
4. 创建Pull Request

## 许可证

本项目采用GNU通用公共许可证第3版（GPL v3），详见LICENSE文件。

### GPL v3许可证要点
- **自由使用**：您可以自由地运行、学习、修改和分发本软件
- **开源要求**：如果您分发本软件的修改版本，必须以相同的GPL v3许可证开源
- **无担保**：本软件按"原样"提供，不提供任何形式的担保
- **版权保护**：确保所有用户都能获得自由软件的权利

更多详情请参阅：https://www.gnu.org/licenses/gpl-3.0.html

## 联系方式

- 项目主页: https://github.com/dlogcover/dlogcover
- 问题反馈: https://github.com/dlogcover/dlogcover/issues
- 邮箱: dlogcover@example.com

## 更新日志

### v0.1.0 (当前版本)
**项目初始化和核心功能实现**
- ✅ 完整的AST分析器架构，支持6种分析器类型
- ✅ 多层次覆盖率分析：函数、分支、异常、关键路径
- ✅ 灵活的日志系统支持：Qt日志和自定义日志函数
- ✅ 多格式报告输出：文本和JSON格式
- ✅ 完善的配置系统：支持JSON配置文件和命令行参数
- ✅ 高质量代码：C++17标准，完整的单元测试覆盖
- ✅ 完整的构建系统：CMake + 构建脚本
- ✅ 29个头文件，模块化设计
- ✅ 代码覆盖率检查：要求80%+覆盖率
- ✅ 现代C++特性：智能指针、RAII、异常安全

**技术特性**
- 基于Clang/LLVM的AST分析
- 支持多种Clang库组件
- nlohmann/json配置文件处理
- GoogleTest单元测试框架
- lcov代码覆盖率报告

**构建和部署**
- CMake构建系统，支持Debug和Release模式
- 便捷的build.sh构建脚本
- 完整流程：编译→测试→覆盖率统计
- 支持多种Linux发行版
- 可配置的安装路径

**开发计划**
- 🔄 GUI界面开发(基于DTK/Qt5)
- 🔄 持续集成/部署配置
- 🔄 性能优化和大型项目支持
- 🔄 插件系统架构
- 🔄 IDE集成支持
