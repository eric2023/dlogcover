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

#### 编译时依赖（必需）

##### 核心依赖
- **LLVM/Clang开发库** (≥6.0): 用于C++ AST分析和代码解析
- **nlohmann/json** (≥3.0): JSON配置文件处理
- **GoogleTest** (≥1.8): 单元测试框架（项目强制要求80%+测试覆盖率）
- **CMake** (≥3.10): 构建系统
- **C++17编译器**: GCC 7.0+ 或 Clang 6.0+

##### Clang库组件要求
项目需要以下Clang库组件（通过libclang-dev安装）：
- `clangTooling` - 代码分析工具支持
- `clangBasic` - 基础Clang功能
- `clangAST` - 抽象语法树处理
- `clangASTMatchers` - AST模式匹配
- `clangFrontend` - 编译器前端
- `clangLex` - 词法分析
- `clangParse` - 语法分析
- `clangSema` - 语义分析
- `clangAnalysis` - 静态分析
- `clangDriver` - 编译器驱动
- `clangEdit` - 代码编辑支持
- `clangRewrite` - 代码重写支持
- `clangSerialization` - 序列化支持
- `clangOpenMP` - OpenMP支持（可选）

#### 运行时依赖（可选但推荐）
- **lcov** (≥1.13): 代码覆盖率报告生成
- **genhtml**: HTML覆盖率报告生成（lcov包含）

#### Ubuntu/Debian 安装命令

##### 方法1：一键安装所有依赖（推荐）
```bash
# 更新包管理器
sudo apt-get update

# 安装所有必需依赖
sudo apt-get install -y \
    build-essential \
    cmake \
    libclang-dev \
    llvm-dev \
    clang \
    clang-tools \
    libc++-dev \
    libc++abi-dev \
    nlohmann-json3-dev \
    libgtest-dev \
    lcov \
    gcov

# 验证安装
clang --version
cmake --version
lcov --version
```

##### 方法2：分步安装
```bash
# 1. 基础构建工具
sudo apt-get update
sudo apt-get install -y build-essential cmake

# 2. LLVM/Clang开发库（核心依赖）
sudo apt-get install -y libclang-dev llvm-dev clang clang-tools

# 3. C++标准库支持
sudo apt-get install -y libc++-dev libc++abi-dev

# 4. JSON库
sudo apt-get install -y nlohmann-json3-dev

# 5. 测试框架（必需）
sudo apt-get install -y libgtest-dev

# 6. 覆盖率工具（推荐）
sudo apt-get install -y lcov gcov

# 7. 可选：额外的开发工具
sudo apt-get install -y gdb valgrind
```

##### 特定版本安装（如需要）
```bash
# 安装特定版本的LLVM/Clang（例如LLVM 14）
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 14

# 安装对应版本的开发库
sudo apt-get install -y libclang-14-dev llvm-14-dev
```

#### CentOS/RHEL/Fedora 安装命令

##### CentOS 8/RHEL 8/Rocky Linux 8
```bash
# 启用PowerTools仓库（CentOS 8）或CodeReady仓库（RHEL 8）
sudo dnf config-manager --set-enabled powertools  # CentOS 8
# 或者
sudo subscription-manager repos --enable codeready-builder-for-rhel-8-x86_64-rpms  # RHEL 8

# 安装EPEL仓库
sudo dnf install -y epel-release

# 安装所有依赖
sudo dnf groupinstall -y "Development Tools"
sudo dnf install -y \
    cmake \
    clang-devel \
    llvm-devel \
    clang \
    clang-tools-extra \
    json-devel \
    gtest-devel \
    lcov \
    gcov
```

##### Fedora
```bash
# 安装所有依赖
sudo dnf groupinstall -y "Development Tools"
sudo dnf install -y \
    cmake \
    clang-devel \
    llvm-devel \
    clang \
    clang-tools-extra \
    json-devel \
    gtest-devel \
    lcov \
    gcov
```

##### CentOS 7/RHEL 7（需要额外步骤）
```bash
# 安装开发工具
sudo yum groupinstall -y "Development Tools"
sudo yum install -y epel-release

# 安装较新版本的CMake
sudo yum install -y cmake3
sudo alternatives --install /usr/bin/cmake cmake /usr/bin/cmake3 20

# 安装LLVM/Clang（可能需要从源码编译或使用第三方仓库）
sudo yum install -y centos-release-scl
sudo yum install -y llvm-toolset-7-clang-devel llvm-toolset-7-llvm-devel

# 其他依赖
sudo yum install -y json-devel gtest-devel lcov
```

#### Arch Linux 安装命令
```bash
# 安装所有依赖
sudo pacman -S --needed \
    base-devel \
    cmake \
    clang \
    llvm \
    nlohmann-json \
    gtest \
    lcov
```

#### macOS 安装命令（使用Homebrew）
```bash
# 安装Homebrew（如果未安装）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install \
    cmake \
    llvm \
    nlohmann-json \
    googletest \
    lcov

# 设置环境变量
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"
```

#### 验证安装
安装完成后，验证关键依赖是否正确安装：

```bash
# 检查编译器版本
gcc --version
clang --version

# 检查CMake版本
cmake --version

# 检查LLVM配置
llvm-config --version
llvm-config --cxxflags
llvm-config --ldflags

# 检查Clang库
pkg-config --exists clang && echo "Clang库已安装" || echo "Clang库未找到"

# 检查JSON库
echo '#include <nlohmann/json.hpp>' | g++ -x c++ -c - && echo "nlohmann/json已安装" || echo "nlohmann/json未找到"

# 检查GoogleTest
echo '#include <gtest/gtest.h>' | g++ -x c++ -c - && echo "GoogleTest已安装" || echo "GoogleTest未找到"

# 检查覆盖率工具
lcov --version
gcov --version
```

#### 常见问题和故障排除

##### 问题1：找不到Clang库
```bash
# 错误信息：Could not find clang libraries
# 解决方案：
sudo apt-get install -y libclang-dev clang-tools
# 或者指定版本
sudo apt-get install -y libclang-12-dev

# 验证安装
find /usr -name "libclang*.so" 2>/dev/null
```

##### 问题2：nlohmann/json未找到
```bash
# 错误信息：nlohmann/json.hpp: No such file or directory
# 解决方案1：使用包管理器
sudo apt-get install -y nlohmann-json3-dev

# 解决方案2：手动下载头文件
sudo mkdir -p /usr/local/include/nlohmann
sudo wget -O /usr/local/include/nlohmann/json.hpp \
    https://github.com/nlohmann/json/releases/latest/download/json.hpp
```

##### 问题3：GoogleTest编译错误
```bash
# 错误信息：gtest/gtest.h: No such file or directory
# Ubuntu/Debian解决方案：
sudo apt-get install -y libgtest-dev
cd /usr/src/gtest
sudo cmake CMakeLists.txt
sudo make
sudo cp lib/*.a /usr/lib

# 或者使用现代方式：
sudo apt-get install -y googletest
```

##### 问题4：CMake版本过低
```bash
# 错误信息：CMake 3.10 or higher is required
# Ubuntu 18.04及更早版本：
sudo apt-get remove cmake
sudo snap install cmake --classic

# 或者从源码编译：
wget https://github.com/Kitware/CMake/releases/download/v3.25.0/cmake-3.25.0.tar.gz
tar -xzf cmake-3.25.0.tar.gz
cd cmake-3.25.0
./bootstrap && make && sudo make install
```

##### 问题5：C++17支持问题
```bash
# 错误信息：This file requires compiler and library support for C++17
# 解决方案：更新编译器
sudo apt-get install -y gcc-9 g++-9
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 90
```

##### 问题6：lcov版本过低
```bash
# 错误信息：lcov version too old
# 解决方案：从源码安装最新版本
wget https://github.com/linux-test-project/lcov/releases/download/v1.16/lcov-1.16.tar.gz
tar -xzf lcov-1.16.tar.gz
cd lcov-1.16
sudo make install
```

##### 问题7：权限问题
```bash
# 错误信息：Permission denied
# 解决方案：确保用户在正确的组中
sudo usermod -a -G sudo $USER
# 重新登录或使用：
newgrp sudo
```

##### 问题8：内存不足
```bash
# 错误信息：virtual memory exhausted
# 解决方案：增加交换空间
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile

# 或者减少并行编译线程：
make -j2  # 而不是 make -j$(nproc)
```

#### 环境变量配置
某些情况下可能需要设置环境变量：

```bash
# 添加到 ~/.bashrc 或 ~/.zshrc
export LLVM_DIR=/usr/lib/llvm-12
export Clang_DIR=/usr/lib/cmake/clang-12
export PATH=$LLVM_DIR/bin:$PATH
export LD_LIBRARY_PATH=$LLVM_DIR/lib:$LD_LIBRARY_PATH

# 重新加载配置
source ~/.bashrc
```

#### Docker环境（推荐用于CI/CD）
如果遇到依赖问题，可以使用Docker环境：

```dockerfile
# Dockerfile.dlogcover
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libclang-dev \
    llvm-dev \
    clang \
    clang-tools \
    nlohmann-json3-dev \
    libgtest-dev \
    lcov \
    gcov \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . .
RUN ./build.sh --test
```

```bash
# 使用Docker构建
docker build -f Dockerfile.dlogcover -t dlogcover .
docker run --rm -v $(pwd):/workspace dlogcover
```

### 构建步骤

#### 前置检查
在开始构建之前，请确保已安装所有依赖：

```bash
# 快速依赖检查脚本
./scripts/check_dependencies.sh  # 如果存在
# 或者手动检查：
clang --version && cmake --version && lcov --version
```

#### 方法1: 使用CMake手动构建

##### 基本构建（Release版本）
```bash
# 创建构建目录
mkdir build && cd build

# 配置项目（Release版本，不包含测试）
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTS=OFF \
      -DENABLE_COVERAGE=OFF \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      ..

# 编译项目
cmake --build . --parallel $(nproc)

# 安装（可选）
sudo cmake --install .
```

##### 开发构建（Debug版本 + 测试 + 覆盖率）
```bash
# 创建构建目录
mkdir build && cd build

# 配置项目（Debug版本，包含测试和覆盖率）
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DBUILD_TESTS=ON \
      -DENABLE_COVERAGE=ON \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      ..

# 编译项目和测试
cmake --build . --parallel $(nproc)

# 运行测试
ctest --verbose --parallel $(nproc)

# 生成覆盖率报告（如果启用了覆盖率）
cmake --build . --target coverage

# 查看覆盖率报告
firefox build/coverage_report/index.html  # 或使用其他浏览器
```

##### 高级构建选项
```bash
# 指定特定的LLVM版本
cmake -DCMAKE_BUILD_TYPE=Release \
      -DLLVM_DIR=/usr/lib/llvm-12/lib/cmake/llvm \
      -DClang_DIR=/usr/lib/cmake/clang-12 \
      ..

# 启用详细编译输出
cmake --build . --verbose

# 只编译特定目标
cmake --build . --target dlogcover

# 并行测试执行
ctest --parallel 4 --output-on-failure
```

#### 方法2: 使用构建脚本（推荐）
项目提供了便捷的构建脚本 `build.sh`，自动处理依赖检查和构建流程：

##### 基本使用
```bash
# 显示帮助信息和所有可用选项
./build.sh --help

# 基本构建（Debug版本，无测试）
./build.sh

# 构建并运行测试
./build.sh --test

# 构建Release版本
./build.sh --release

# 执行完整流程：编译→测试→覆盖率统计→报告生成
./build.sh --full-process

# 清理构建目录并重新构建
./build.sh --clean --test

# 安装到指定路径
./build.sh --install --prefix=/opt/dlogcover
```

##### 高级使用场景
```bash
# 开发者完整工作流
./build.sh --clean --debug --test --full-process

# CI/CD流水线使用
./build.sh --release --test --install --prefix=/usr/local

# 快速验证构建
./build.sh --release

# 性能测试构建
./build.sh --release --install

# 调试构建（包含所有调试信息）
./build.sh --debug --test
```

##### 构建脚本特性
- **自动依赖检查**: 构建前验证所有必需依赖
- **智能并行构建**: 自动检测CPU核心数并优化编译速度
- **覆盖率集成**: 一键生成完整的覆盖率报告
- **错误处理**: 详细的错误信息和建议解决方案
- **进度显示**: 实时显示构建进度和耗时统计

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

#### 标准构建输出
成功构建后将在 `build/` 目录下生成：

```
build/
├── bin/
│   └── dlogcover              # 主程序可执行文件
├── lib/                       # 静态库文件（如果有）
├── compile_commands.json      # 编译命令数据库（用于IDE和分析工具）
├── CMakeCache.txt            # CMake缓存文件
└── CMakeFiles/               # CMake内部文件
```

#### 测试构建输出（使用 --test）
```
build/
├── bin/
│   ├── dlogcover              # 主程序
│   └── tests/                 # 测试可执行文件
│       ├── unit_tests         # 单元测试
│       └── integration_tests  # 集成测试
├── Testing/                   # CTest输出
└── test_results/              # 测试结果文件
```

#### 覆盖率构建输出（使用 --full-process）
```
build/
├── bin/dlogcover              # 主程序
├── coverage.info              # 原始覆盖率数据
├── coverage.filtered.info     # 过滤后的覆盖率数据
├── coverage_report/           # HTML覆盖率报告
│   ├── index.html            # 主报告页面
│   ├── src/                  # 源码覆盖率详情
│   └── tests/                # 测试覆盖率详情
└── *.gcda, *.gcno            # GCC覆盖率数据文件
```

#### 安装输出（使用 --install）
默认安装到 `/usr/local/`（可通过 --prefix 修改）：

```
/usr/local/
├── bin/
│   └── dlogcover              # 主程序
├── share/
│   └── dlogcover/
│       ├── dlogcover.json     # 示例配置文件
│       └── examples/          # 使用示例
└── include/                   # 头文件（如果作为库使用）
    └── dlogcover/
```

#### 验证构建结果
```bash
# 检查可执行文件
ls -la build/bin/dlogcover
file build/bin/dlogcover

# 运行版本检查
./build/bin/dlogcover --version

# 检查依赖库
ldd build/bin/dlogcover

# 验证覆盖率报告（如果生成）
ls -la build/coverage_report/
```

## 快速开始

### 一分钟快速体验

#### Ubuntu/Debian 用户
```bash
# 1. 安装依赖（一键安装）
sudo apt-get update && sudo apt-get install -y \
    build-essential cmake libclang-dev llvm-dev clang \
    nlohmann-json3-dev libgtest-dev lcov

# 2. 克隆项目
git clone https://github.com/dlogcover/dlogcover.git
cd dlogcover

# 3. 构建和测试
./build.sh --full-process

# 4. 运行示例
./build/bin/dlogcover --help
./build/bin/dlogcover -d ./examples
```

#### 其他系统用户
```bash
# 1. 参考上面的"系统依赖项"部分安装依赖
# 2. 克隆和构建
git clone https://github.com/dlogcover/dlogcover.git
cd dlogcover
./build.sh --full-process
```

### 验证安装
```bash
# 检查版本
./build/bin/dlogcover --version

# 查看覆盖率报告
firefox build/coverage_report/index.html

# 运行示例分析
./build/bin/dlogcover -d ./src -f json -o coverage_report.json
```

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
- `cmake`: CMake自动配置选项
  - `enabled`: 是否启用CMake参数自动检测
  - `cmake_lists_path`: CMakeLists.txt文件路径（空则自动查找）
  - `target_name`: 目标名称（空则使用全局参数）
  - `use_compile_commands`: 是否使用compile_commands.json
  - `compile_commands_path`: compile_commands.json路径
  - `verbose_logging`: 是否启用详细日志

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

## CMake集成功能

### 自动编译参数检测
DLogCover支持自动从CMakeLists.txt文件中提取编译参数，大大简化了配置过程。

#### 功能特性
- **自动查找CMakeLists.txt**: 从当前文件目录向上递归查找
- **全局参数提取**: 自动提取项目级别的编译设置
- **目标特定参数**: 支持提取特定目标的编译参数
- **智能参数合并**: 避免重复参数，优化编译命令
- **详细日志支持**: 可启用详细日志查看解析过程

#### 支持的CMake命令
- `project()` - 项目名称和版本
- `set(CMAKE_CXX_STANDARD)` - C++标准设置
- `include_directories()` - 全局包含目录
- `add_definitions()` - 全局编译定义
- `add_executable()` / `add_library()` - 目标定义
- `target_include_directories()` - 目标包含目录
- `target_compile_definitions()` - 目标编译定义
- `target_compile_options()` - 目标编译选项
- `target_link_libraries()` - 目标链接库
- `find_package()` - 包查找（基础支持）

#### 配置示例
```json
{
    "scan": {
        "cmake": {
            "enabled": true,
            "cmake_lists_path": "",
            "target_name": "my_app",
            "use_compile_commands": true,
            "compile_commands_path": "./build/compile_commands.json",
            "verbose_logging": false
        }
    }
}
```

#### 使用场景

**场景1: 自动检测项目配置**
```json
{
    "scan": {
        "cmake": {
            "enabled": true
        }
    }
}
```
DLogCover将自动查找CMakeLists.txt并提取全局编译参数。

**场景2: 分析特定目标**
```json
{
    "scan": {
        "cmake": {
            "enabled": true,
            "target_name": "my_library"
        }
    }
}
```
只提取指定目标的编译参数，包括目标特定的包含目录、编译定义等。

**场景3: 使用编译数据库**
```json
{
    "scan": {
        "cmake": {
            "enabled": true,
            "use_compile_commands": true,
            "compile_commands_path": "./build/compile_commands.json"
        }
    }
}
```
优先使用CMake生成的compile_commands.json文件。

**场景4: 调试CMake解析**
```json
{
    "scan": {
        "cmake": {
            "enabled": true,
            "verbose_logging": true
        }
    }
}
```
启用详细日志，查看CMake解析的详细过程。

#### 工作流程
1. **查找CMakeLists.txt**: 从源文件目录开始向上查找
2. **解析CMake命令**: 提取项目配置、目标定义等
3. **生成编译参数**: 转换为Clang可用的编译参数
4. **参数合并**: 与用户配置的参数合并，避免重复
5. **AST生成**: 使用完整的编译参数生成AST

#### 优势
- **零配置**: 对于标准CMake项目，无需手动配置编译参数
- **准确性**: 使用项目实际的编译设置，确保AST分析准确性
- **灵活性**: 支持全局和目标特定的参数提取
- **兼容性**: 与现有配置系统完全兼容，可以混合使用

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
- 邮箱: eric2023@163.com

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
