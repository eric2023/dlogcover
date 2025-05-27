# DLogCover 系统架构模式

## 整体架构设计

### 架构原则

DLogCover采用分层架构和模块化设计，遵循以下核心原则：

#### 单一职责原则（SRP）
- 每个模块专注于特定的功能领域
- CLI模块仅处理命令行解析
- AST分析器专注于代码结构分析
- 报告生成器专注于结果输出

#### 开闭原则（OCP）
- 使用策略模式支持多种报告格式
- 通过配置系统支持不同日志函数
- 预留扩展点支持新的分析类型

#### 依赖倒置原则（DIP）
- 高层模块不依赖低层模块实现
- 使用抽象接口定义模块间契约
- 通过依赖注入管理组件关系

### 系统分层架构

```
┌─────────────────────────────────┐
│         用户接口层                │
│     (CLI + Configuration)       │
└─────────────────────────────────┘
           │
┌─────────────────────────────────┐
│         应用服务层                │
│     (Orchestration Layer)      │
└─────────────────────────────────┘
           │
┌─────────────────────────────────┐
│         业务逻辑层                │
│  (AST Analysis + Log Detection) │
└─────────────────────────────────┘
           │
┌─────────────────────────────────┐
│         数据访问层                │
│   (File System + LLVM APIs)    │
└─────────────────────────────────┘
```

## 核心组件架构

### 1. 命令行接口模块 (CLI)

#### 设计模式：Command Pattern + Builder Pattern

```cpp
class CommandLineParser {
private:
    OptionsBuilder builder_;
    std::vector<std::unique_ptr<ICommand>> commands_;
    
public:
    Result<Options> parse(int argc, char** argv);
    void registerCommand(std::unique_ptr<ICommand> command);
};

class OptionsBuilder {
public:
    OptionsBuilder& setDirectory(const std::string& dir);
    OptionsBuilder& setOutput(const std::string& output);
    OptionsBuilder& setConfigFile(const std::string& config);
    Options build();
};
```

#### 关键决策
- **使用Builder模式**：简化复杂的命令行选项构建过程
- **命令注册机制**：支持插件式扩展新的命令
- **错误处理策略**：使用Result类型封装解析错误

### 2. 配置管理模块 (Config)

#### 设计模式：Strategy Pattern + Factory Pattern

```cpp
class ConfigManager {
private:
    std::unique_ptr<IConfigLoader> loader_;
    std::unique_ptr<IConfigValidator> validator_;
    Config config_;
    
public:
    Result<bool> loadConfig(const std::string& path);
    Result<bool> mergeWithOptions(const Options& options);
    const Config& getConfig() const;
};

class IConfigLoader {
public:
    virtual Result<Config> load(const std::string& path) = 0;
};

class JsonConfigLoader : public IConfigLoader {
public:
    Result<Config> load(const std::string& path) override;
};
```

#### 关键决策
- **策略模式支持多格式**：当前支持JSON，未来可扩展YAML、TOML
- **分层配置合并**：全局配置 < 用户配置 < 项目配置 < 命令行参数
- **配置验证机制**：独立的验证器确保配置完整性和正确性

### 3. 源文件管理模块 (SourceManager)

#### 设计模式：Iterator Pattern + Visitor Pattern

```cpp
class SourceManager {
private:
    Config config_;
    std::vector<std::string> sourceFiles_;
    std::unique_ptr<IFileFilter> filter_;
    
public:
    Result<bool> collectSourceFiles();
    Iterator<std::string> begin();
    Iterator<std::string> end();
    size_t getFileCount() const;
};

class IFileFilter {
public:
    virtual bool shouldInclude(const std::string& filePath) = 0;
};

class ExtensionFilter : public IFileFilter {
private:
    std::set<std::string> allowedExtensions_;
    
public:
    bool shouldInclude(const std::string& filePath) override;
};
```

#### 关键决策
- **迭代器模式**：支持懒加载和流式处理大量文件
- **过滤器链**：组合多种过滤条件（扩展名、路径模式、大小限制）
- **并行文件发现**：使用std::filesystem的并行算法提高性能

### 4. AST分析引擎 (ASTAnalyzer)

#### 设计模式：Visitor Pattern + Template Method Pattern

```cpp
class ASTAnalyzer {
private:
    clang::tooling::ClangTool tool_;
    Config config_;
    std::unique_ptr<ASTConsumer> consumer_;
    
public:
    Result<bool> analyzeFile(const std::string& filePath);
    Result<bool> analyzeAll();
    const std::vector<ASTNodeInfo>& getAnalysisResults() const;
};

class DLogCoverASTVisitor : public clang::RecursiveASTVisitor<DLogCoverASTVisitor> {
private:
    clang::ASTContext& context_;
    std::vector<ASTNodeInfo>& results_;
    
public:
    bool VisitFunctionDecl(clang::FunctionDecl* decl);
    bool VisitIfStmt(clang::IfStmt* stmt);
    bool VisitCXXTryStmt(clang::CXXTryStmt* stmt);
    bool VisitForStmt(clang::ForStmt* stmt);
    bool VisitWhileStmt(clang::WhileStmt* stmt);
};
```

#### 关键决策
- **访问者模式**：解耦AST节点类型和处理逻辑
- **模板方法模式**：定义分析流程框架，子类实现具体步骤
- **增量分析支持**：缓存分析结果，仅重新分析修改的文件

### 5. 日志识别模块 (LogIdentifier)

#### 设计模式：Strategy Pattern + Chain of Responsibility

```cpp
class LogIdentifier {
private:
    std::vector<std::unique_ptr<ILogDetectionStrategy>> strategies_;
    Config config_;
    
public:
    Result<std::vector<LogCallSite>> identifyLogCalls(const ASTNodeInfo& node);
    void addStrategy(std::unique_ptr<ILogDetectionStrategy> strategy);
};

class ILogDetectionStrategy {
public:
    virtual std::vector<LogCallSite> detect(const clang::CallExpr* call) = 0;
    virtual bool canHandle(const clang::CallExpr* call) = 0;
};

class QtLogDetectionStrategy : public ILogDetectionStrategy {
private:
    std::set<std::string> qtLogFunctions_;
    
public:
    std::vector<LogCallSite> detect(const clang::CallExpr* call) override;
    bool canHandle(const clang::CallExpr* call) override;
};

class CustomLogDetectionStrategy : public ILogDetectionStrategy {
private:
    std::map<std::string, LogLevel> customLogFunctions_;
    
public:
    std::vector<LogCallSite> detect(const clang::CallExpr* call) override;
    bool canHandle(const clang::CallExpr* call) override;
};
```

#### 关键决策
- **策略模式**：支持不同类型的日志系统（Qt、自定义、未来的其他框架）
- **责任链模式**：按优先级尝试不同的检测策略
- **可配置识别规则**：通过配置文件扩展支持的日志函数

### 6. 覆盖率计算模块 (CoverageCalculator)

#### 设计模式：Strategy Pattern + Observer Pattern

```cpp
class CoverageCalculator {
private:
    std::vector<std::unique_ptr<ICoverageStrategy>> strategies_;
    std::vector<ICoverageObserver*> observers_;
    
public:
    Result<CoverageStats> calculateCoverage(
        const std::vector<ASTNodeInfo>& nodes,
        const std::vector<LogCallSite>& logCalls
    );
    
    void addStrategy(std::unique_ptr<ICoverageStrategy> strategy);
    void addObserver(ICoverageObserver* observer);
};

class ICoverageStrategy {
public:
    virtual CoverageMetrics calculate(
        const std::vector<ASTNodeInfo>& nodes,
        const std::vector<LogCallSite>& logCalls
    ) = 0;
    virtual std::string getMetricName() const = 0;
};

class FunctionCoverageStrategy : public ICoverageStrategy {
public:
    CoverageMetrics calculate(
        const std::vector<ASTNodeInfo>& nodes,
        const std::vector<LogCallSite>& logCalls
    ) override;
    std::string getMetricName() const override { return "function_coverage"; }
};
```

#### 关键决策
- **策略模式**：支持多种覆盖率计算方法（函数级、分支级、路径级）
- **观察者模式**：实时通知计算进度，支持进度条显示
- **可扩展指标系统**：易于添加新的覆盖率指标类型

### 7. 报告生成模块 (Reporter)

#### 设计模式：Strategy Pattern + Builder Pattern + Template Method

```cpp
class Reporter {
private:
    std::unique_ptr<IReporterStrategy> strategy_;
    Config config_;
    
public:
    Result<bool> generateReport(
        const CoverageStats& stats,
        const std::string& outputPath
    );
    
    void setStrategy(std::unique_ptr<IReporterStrategy> strategy);
};

class IReporterStrategy {
public:
    virtual Result<std::string> generateReport(const CoverageStats& stats) = 0;
    virtual std::string getFileExtension() const = 0;
    virtual std::string getContentType() const = 0;
};

class TextReporterStrategy : public IReporterStrategy {
public:
    Result<std::string> generateReport(const CoverageStats& stats) override;
    std::string getFileExtension() const override { return ".txt"; }
    std::string getContentType() const override { return "text/plain"; }
};

class JsonReporterStrategy : public IReporterStrategy {
public:
    Result<std::string> generateReport(const CoverageStats& stats) override;
    std::string getFileExtension() const override { return ".json"; }
    std::string getContentType() const override { return "application/json"; }
};
```

#### 关键决策
- **策略模式**：支持多种报告格式（文本、JSON、未来的HTML、XML）
- **建造者模式**：复杂报告的分步构建
- **模板方法模式**：定义报告生成的通用流程

## 数据流架构

### 数据传递模式

```
Source Files → AST Nodes → Log Calls → Coverage Stats → Reports
     ↓             ↓           ↓            ↓           ↓
  FileInfo    ASTNodeInfo  LogCallSite  CoverageStats  String
```

### 错误传播机制

所有模块使用统一的`Result<T>`类型进行错误处理：

```cpp
template<typename T>
class Result {
private:
    std::optional<T> value_;
    std::optional<Error> error_;
    
public:
    bool isSuccess() const;
    bool isError() const;
    const T& getValue() const;
    const Error& getError() const;
    
    // 链式操作支持
    template<typename F>
    auto map(F&& func) -> Result<decltype(func(value_.value()))>;
    
    template<typename F>
    Result<T> flatMap(F&& func);
};
```

### 进度反馈机制

```cpp
class ProgressTracker {
private:
    size_t totalSteps_;
    size_t currentStep_;
    std::vector<IProgressObserver*> observers_;
    
public:
    void setTotalSteps(size_t steps);
    void nextStep(const std::string& description);
    void addObserver(IProgressObserver* observer);
};

class IProgressObserver {
public:
    virtual void onProgressUpdate(size_t current, size_t total, const std::string& description) = 0;
};
```

## 性能优化策略

### 内存管理模式

#### 1. RAII资源管理
- 使用智能指针管理Clang AST对象生命周期
- 及时释放不再需要的AST单元
- 避免循环引用导致的内存泄漏

#### 2. 对象池模式
```cpp
class ASTNodePool {
private:
    std::vector<std::unique_ptr<ASTNodeInfo>> pool_;
    std::queue<ASTNodeInfo*> available_;
    
public:
    ASTNodeInfo* acquire();
    void release(ASTNodeInfo* node);
    void clear();
};
```

#### 3. 写时复制（COW）
- 对于大型数据结构使用COW策略
- 减少不必要的深拷贝操作
- 优化函数参数传递

### 并发处理模式

#### 1. 文件级并行分析
```cpp
class ParallelAnalyzer {
private:
    ThreadPool threadPool_;
    std::atomic<size_t> completedFiles_;
    
public:
    Result<bool> analyzeFilesInParallel(const std::vector<std::string>& files);
};
```

#### 2. 流水线处理
- AST分析和日志识别可以并行进行
- 使用生产者-消费者模式处理文件队列
- 重叠I/O操作和计算操作

### 缓存策略

#### 1. AST缓存
```cpp
class ASTCache {
private:
    std::unordered_map<std::string, std::pair<time_t, ASTNodeInfo>> cache_;
    size_t maxCacheSize_;
    
public:
    std::optional<ASTNodeInfo> get(const std::string& filePath, time_t modTime);
    void put(const std::string& filePath, time_t modTime, const ASTNodeInfo& info);
    void evictOldEntries();
};
```

#### 2. 增量分析
- 跟踪文件修改时间
- 仅重新分析修改过的文件
- 合并增量结果与缓存结果

## 可扩展性设计

### 插件机制

```cpp
class PluginManager {
private:
    std::vector<std::unique_ptr<IPlugin>> plugins_;
    
public:
    void loadPlugin(const std::string& pluginPath);
    void registerPlugin(std::unique_ptr<IPlugin> plugin);
    void initializePlugins();
};

class IPlugin {
public:
    virtual std::string getName() const = 0;
    virtual std::string getVersion() const = 0;
    virtual void initialize(const Config& config) = 0;
    virtual void shutdown() = 0;
};
```

### 扩展点设计

1. **自定义日志检测策略**
2. **新的覆盖率计算方法**
3. **自定义报告格式**
4. **额外的AST分析器**
5. **自定义配置加载器**

## 质量保证架构

### 测试策略分层

```
┌─────────────────────────────────┐
│        端到端测试              │  ← 完整工作流测试
└─────────────────────────────────┘
┌─────────────────────────────────┐
│        集成测试                │  ← 模块间协作测试
└─────────────────────────────────┘
┌─────────────────────────────────┐
│        单元测试                │  ← 单个类/函数测试
└─────────────────────────────────┘
```

### 错误恢复机制

- **优雅降级**：部分文件分析失败时继续处理其他文件
- **错误聚合**：收集所有错误信息统一报告
- **状态回滚**：关键操作失败时恢复到安全状态
- **资源清理**：异常情况下确保资源正确释放

这种架构设计确保了DLogCover具有良好的可维护性、可扩展性和性能表现，为长期的功能演进和优化提供了坚实基础。 