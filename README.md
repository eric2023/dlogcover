# DLogCover - C++ 日志覆盖率分析工具

DLogCover 是一个专门用于分析 C++ 项目中日志函数覆盖率的工具。它基于 `compile_commands.json` 提供准确的 AST 分析，支持 Qt 和自定义日志函数的识别与分析。

## 主要特性

- **基于 compile_commands.json 的准确分析** - 使用项目实际的编译参数，确保 AST 解析准确性
- **自动化 CMake 集成** - 自动生成和解析 compile_commands.json 文件
- **多种日志函数支持** - 支持 Qt 日志函数（qDebug、qInfo 等）和自定义日志函数
- **简化的配置管理** - 移除复杂的手动编译参数配置，专注于项目结构配置
- **详细的覆盖率报告** - 提供函数级别的日志覆盖率分析

## 快速开始

### 1. 基本使用

对于使用 CMake 的 C++ 项目，DLogCover 可以自动处理大部分配置：

```bash
# 分析当前目录的项目
./build/bin/dlogcover

# 分析指定项目
./build/bin/dlogcover --config project/dlogcover.json

# 指定输出文件
./build/bin/dlogcover --output coverage_report.txt --log-path analysis.log
```

### 2. 配置文件

DLogCover 使用简化的 JSON 配置文件。如果项目目录中没有 `dlogcover.json`，工具会使用默认配置。

#### 默认配置文件示例 (`dlogcover.json`)

```json
{
    "project": {
        "name": "MyProject",
        "directory": ".",
        "build_directory": "./build"
    },
    "scan": {
        "directories": ["./src"],
        "file_extensions": [".cpp", ".cc", ".cxx", ".c"],
        "exclude_patterns": ["*test*", "*Test*", "*/tests/*", "*/build/*", "*/.git/*"]
    },
    "compile_commands": {
        "path": "./build/compile_commands.json",
        "auto_generate": true,
        "cmake_args": ["-DCMAKE_EXPORT_COMPILE_COMMANDS=1", "-DCMAKE_BUILD_TYPE=Debug"]
    },
    "output": {
        "report_file": "coverage_report.txt",
        "log_file": "analysis.log",
        "log_level": "INFO"
    }
}
```

### 3. 工作流程

DLogCover 的分析流程如下：

1. **检查配置** - 加载配置文件或使用默认配置
2. **生成编译数据库** - 如果启用 `auto_generate`，自动运行 CMake 生成 `compile_commands.json`
3. **解析编译命令** - 从 `compile_commands.json` 中提取每个源文件的编译参数
4. **AST 分析** - 使用准确的编译参数创建 AST 并分析日志函数
5. **生成报告** - 输出详细的覆盖率分析报告

## 配置说明

### 项目配置 (project)

- `name`: 项目名称
- `directory`: 项目根目录
- `build_directory`: CMake 构建目录

### 扫描配置 (scan)

- `directories`: 要扫描的源代码目录列表（相对于项目根目录）
- `file_extensions`: 要分析的文件扩展名列表
- `exclude_patterns`: 排除的文件/目录模式

### 编译命令配置 (compile_commands)

- `path`: `compile_commands.json` 文件路径
- `auto_generate`: 是否自动生成 `compile_commands.json`
- `cmake_args`: 传递给 CMake 的额外参数

### 输出配置 (output)

- `report_file`: 覆盖率报告文件名
- `log_file`: 分析日志文件名
- `log_level`: 日志级别 (DEBUG, INFO, WARNING, ERROR, FATAL)

## 支持的日志函数

### Qt 日志函数

- 标准函数: `qDebug`, `qInfo`, `qWarning`, `qCritical`, `qFatal`
- 分类函数: `qCDebug`, `qCInfo`, `qCWarning`, `qCCritical`

### 自定义日志函数

- Debug: `logDebug`, `LOG_DEBUG`, `LOG_DEBUG_FMT`
- Info: `logInfo`, `LOG_INFO`, `LOG_INFO_FMT`
- Warning: `logWarning`, `LOG_WARNING`, `LOG_WARNING_FMT`
- Error: `logError`, `LOG_ERROR`, `LOG_ERROR_FMT`
- Fatal: `logFatal`, `LOG_FATAL`, `LOG_FATAL_FMT`

## 命令行选项

```bash
Usage: dlogcover [options]

Options:
  --config <file>          配置文件路径 (默认: ./dlogcover.json)
  --output <file>          输出报告文件路径
  --log-path <file>        日志文件路径
  --log-level <level>      日志级别 (debug|info|warning|error|fatal)
  --directory <path>       项目目录路径
  --exclude <pattern>      排除模式 (可多次使用)
  --help                   显示帮助信息
  --version                显示版本信息
```

## 系统要求

- **CMake 3.10+** - 用于生成 compile_commands.json
- **Clang/LLVM** - 用于 AST 分析
- **C++17 编译器** - 用于编译 DLogCover 本身

## 构建说明

```bash
# 克隆仓库
git clone <repository-url>
cd dlogcover

# 创建构建目录
mkdir build && cd build

# 配置和编译
cmake ..
make -j$(nproc)

# 运行测试
make test
```

## 示例项目分析

以 deepin-draw 项目为例：

```bash
# 1. 创建配置文件
cat > deepin-draw/dlogcover.json << EOF
{
    "project": {
        "name": "deepin-draw",
        "directory": "./deepin-draw",
        "build_directory": "./deepin-draw/build"
    },
    "scan": {
        "directories": ["./src"],
        "file_extensions": [".cpp", ".cc", ".cxx"],
        "exclude_patterns": ["*test*", "*Test*", "*/tests/*"]
    },
    "compile_commands": {
        "path": "./deepin-draw/build/compile_commands.json",
        "auto_generate": true,
        "cmake_args": ["-DCMAKE_EXPORT_COMPILE_COMMANDS=1"]
    },
    "output": {
        "report_file": "deepin-draw_report.txt",
        "log_file": "deepin-draw_analysis.log",
        "log_level": "INFO"
    }
}
EOF

# 2. 运行分析
./build/bin/dlogcover --config deepin-draw/dlogcover.json

# 3. 查看结果
cat deepin-draw_report.txt
```

## 故障排除

### 常见问题

1. **AST 编译错误**
   - 确保项目可以正常编译
   - 检查 `compile_commands.json` 是否正确生成
   - 验证 CMake 参数是否正确

2. **找不到源文件**
   - 检查 `scan.directories` 配置
   - 确认文件路径相对于项目根目录

3. **函数覆盖率显示 N/A (0/0)**
   - 这通常表示 AST 解析失败
   - 对于不在构建系统中的文件，工具会自动查找同名文件的编译参数
   - 检查是否存在同名的 `.cpp` 文件在构建系统中

4. **编译命令生成失败**
   - 确保 CMake 版本 >= 3.10
   - 检查 `build_directory` 路径是否正确
   - 验证 CMake 配置是否成功

### 高级配置

#### 同名文件查找功能

当源文件不在构建系统中时，DLogCover 会自动查找同名文件的编译参数：

```cpp
// 例如：./deepin-draw/src/application.cpp 不在构建系统中
// 工具会查找 ./deepin-draw/src/deepin-draw/drawfiles/application.cpp 的编译参数
```

这个功能确保即使是独立的源文件也能得到正确的 AST 分析。

#### 自定义编译参数

如果需要为特定文件指定编译参数，可以在配置中添加：

```json
{
    "compile_commands": {
        "custom_args": {
            "specific_file.cpp": ["-I/custom/include", "-DCUSTOM_DEFINE"]
        }
    }
}
```

## 技术架构

### 核心组件

1. **ConfigManager** - 配置管理器，处理 JSON 配置文件
2. **CompileCommandsManager** - 编译命令管理器，自动生成和解析 compile_commands.json
3. **ASTAnalyzer** - AST 分析器，使用 Clang 进行代码分析
4. **CoverageReporter** - 覆盖率报告生成器

### 工作流程图

```
配置加载 → 编译数据库生成 → AST分析 → 覆盖率计算 → 报告生成
    ↓           ↓              ↓         ↓          ↓
ConfigManager → CompileCommandsManager → ASTAnalyzer → CoverageReporter
```

## 版本历史

### v2.0.0 (当前版本)
- 重构配置架构，移除复杂手动编译参数
- 实现基于 compile_commands.json 的自动化分析
- 添加同名文件查找功能
- 简化配置文件格式
- 提升 AST 解析准确性

### v1.0.0
- 初始版本，基于手动编译参数配置

## 贡献指南

欢迎提交 Issue 和 Pull Request！

### 开发环境设置

```bash
# 安装依赖
sudo apt-get install cmake clang libclang-dev

# 编译调试版本
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# 运行测试
make test
```

## 许可证

本项目采用 MIT 许可证。详见 LICENSE 文件。
   - 确认文件扩展名配置正确
   - 验证排除模式是否过于宽泛

3. **CMake 生成失败**
   - 确保安装了 CMake
   - 检查项目根目录是否有 CMakeLists.txt
   - 验证 CMake 参数是否有效

### 调试技巧

```bash
# 启用详细日志
./build/bin/dlogcover --log-level debug

# 检查 compile_commands.json
cat project/build/compile_commands.json | jq '.[0]'

# 验证配置文件
./build/bin/dlogcover --config project/dlogcover.json --log-level debug
```

## 贡献指南

欢迎提交 Issue 和 Pull Request！请确保：

1. 代码符合项目的编码规范
2. 添加适当的测试用例
3. 更新相关文档

## 许可证

本项目采用 [MIT 许可证](LICENSE)。
