package main

import (
	"encoding/json"
	"fmt"
	"go/ast"
	"go/parser"
	"go/token"
	"io/ioutil"
	"os"
	"strings"
)

// AnalysisRequest 分析请求结构
type AnalysisRequest struct {
	FilePath string                 `json:"file_path"`
	Config   GoAnalysisConfig       `json:"config"`
}

// GoAnalysisConfig Go分析配置
type GoAnalysisConfig struct {
	StandardLog StandardLogConfig `json:"standard_log"`
	Logrus      LogrusConfig      `json:"logrus"`
	Zap         ZapConfig         `json:"zap"`
	Golib       GolibConfig       `json:"golib"`
}

// StandardLogConfig 标准库log配置
type StandardLogConfig struct {
	Enabled   bool     `json:"enabled"`
	Functions []string `json:"functions"`
}

// LogrusConfig Logrus日志库配置
type LogrusConfig struct {
	Enabled   bool     `json:"enabled"`
	Functions []string `json:"functions"`
}

// ZapConfig Zap日志库配置
type ZapConfig struct {
	Enabled           bool     `json:"enabled"`
	LoggerFunctions   []string `json:"logger_functions"`
	SugaredFunctions  []string `json:"sugared_functions"`
}

// GolibConfig Golib日志库配置
type GolibConfig struct {
	Enabled   bool     `json:"enabled"`
	Functions []string `json:"functions"`
}

// AnalysisResult 分析结果结构
type AnalysisResult struct {
	Success   bool           `json:"success"`
	Error     string         `json:"error,omitempty"`
	FilePath  string         `json:"file_path"`
	Functions []FunctionInfo `json:"functions"`
	LogCalls  []LogCallInfo  `json:"log_calls"`
	Statistics Statistics    `json:"statistics"`
}

// FunctionInfo 函数信息
type FunctionInfo struct {
	Name       string        `json:"name"`
	Line       int           `json:"line"`
	Column     int           `json:"column"`
	EndLine    int           `json:"end_line"`
	EndColumn  int           `json:"end_column"`
	HasLogging bool          `json:"has_logging"`
	LogCalls   []LogCallInfo `json:"log_calls"`
}

// LogCallInfo 日志调用信息
type LogCallInfo struct {
	FunctionName string `json:"function_name"`
	Library      string `json:"library"`
	Level        string `json:"level"`
	Line         int    `json:"line"`
	Column       int    `json:"column"`
	Message      string `json:"message,omitempty"`
}

// Statistics 统计信息
type Statistics struct {
	TotalFunctions int `json:"total_functions"`
	TotalLogCalls  int `json:"total_log_calls"`
	LibraryStats   map[string]int `json:"library_stats"`
}

// GoAnalyzer Go代码分析器
type GoAnalyzer struct {
	config   GoAnalysisConfig
	fileSet  *token.FileSet
	logFuncs map[string]LogLibrary
}

// LogLibrary 日志库信息
type LogLibrary struct {
	Name      string
	Functions map[string]string // function -> level
}

// NewGoAnalyzer 创建Go分析器
func NewGoAnalyzer(config GoAnalysisConfig) *GoAnalyzer {
	analyzer := &GoAnalyzer{
		config:   config,
		fileSet:  token.NewFileSet(),
		logFuncs: make(map[string]LogLibrary),
	}
	
	analyzer.initializeLogFunctions()
	return analyzer
}

// initializeLogFunctions 初始化日志函数映射
func (ga *GoAnalyzer) initializeLogFunctions() {
	// 标准库log
	if ga.config.StandardLog.Enabled {
		stdLib := LogLibrary{
			Name:      "standard",
			Functions: make(map[string]string),
		}
		for _, fn := range ga.config.StandardLog.Functions {
			level := "info"
			if strings.Contains(strings.ToLower(fn), "fatal") {
				level = "fatal"
			} else if strings.Contains(strings.ToLower(fn), "panic") {
				level = "panic"
			}
			stdLib.Functions["log."+fn] = level
		}
		ga.logFuncs["log"] = stdLib
	}
	
	// Logrus
	if ga.config.Logrus.Enabled {
		logrusLib := LogLibrary{
			Name:      "logrus",
			Functions: make(map[string]string),
		}
		for _, fn := range ga.config.Logrus.Functions {
			level := strings.ToLower(fn)
			if level == "warning" {
				level = "warn"
			}
			logrusLib.Functions["logrus."+fn] = level
		}
		ga.logFuncs["logrus"] = logrusLib
	}
	
	// Zap Logger
	if ga.config.Zap.Enabled {
		zapLib := LogLibrary{
			Name:      "zap",
			Functions: make(map[string]string),
		}
		for _, fn := range ga.config.Zap.LoggerFunctions {
			level := strings.ToLower(fn)
			if level == "dpanic" {
				level = "panic"
			}
			zapLib.Functions["logger."+fn] = level
		}
		for _, fn := range ga.config.Zap.SugaredFunctions {
			level := strings.ToLower(strings.TrimSuffix(fn, "f"))
			if level == "dpanic" {
				level = "panic"
			}
			zapLib.Functions["sugar."+fn] = level
		}
		ga.logFuncs["zap"] = zapLib
	}
	
	// Golib
	if ga.config.Golib.Enabled {
		golibLib := LogLibrary{
			Name:      "golib",
			Functions: make(map[string]string),
		}
		for _, fn := range ga.config.Golib.Functions {
			level := "info" // 默认级别
			golibLib.Functions["golib."+fn] = level
		}
		ga.logFuncs["golib"] = golibLib
	}
}

// AnalyzeFile 分析Go文件
func (ga *GoAnalyzer) AnalyzeFile(filePath string) (*AnalysisResult, error) {
	result := &AnalysisResult{
		FilePath:   filePath,
		Functions:  []FunctionInfo{},
		LogCalls:   []LogCallInfo{},
		Statistics: Statistics{
			LibraryStats: make(map[string]int),
		},
	}
	
	// 解析Go文件
	src, err := ioutil.ReadFile(filePath)
	if err != nil {
		return nil, fmt.Errorf("读取文件失败: %v", err)
	}
	
	file, err := parser.ParseFile(ga.fileSet, filePath, src, parser.ParseComments)
	if err != nil {
		return nil, fmt.Errorf("解析Go文件失败: %v", err)
	}
	
	// 遍历AST节点
	ast.Inspect(file, func(n ast.Node) bool {
		switch node := n.(type) {
		case *ast.FuncDecl:
			if node.Name != nil {
				funcInfo := ga.analyzeFunctionDecl(node)
				result.Functions = append(result.Functions, funcInfo)
				result.LogCalls = append(result.LogCalls, funcInfo.LogCalls...)
			}
		}
		return true
	})
	
	// 计算统计信息
	result.Statistics.TotalFunctions = len(result.Functions)
	result.Statistics.TotalLogCalls = len(result.LogCalls)
	
	for _, logCall := range result.LogCalls {
		result.Statistics.LibraryStats[logCall.Library]++
	}
	
	result.Success = true
	return result, nil
}

// analyzeFunctionDecl 分析函数声明
func (ga *GoAnalyzer) analyzeFunctionDecl(funcDecl *ast.FuncDecl) FunctionInfo {
	pos := ga.fileSet.Position(funcDecl.Pos())
	end := ga.fileSet.Position(funcDecl.End())
	
	funcInfo := FunctionInfo{
		Name:      funcDecl.Name.Name,
		Line:      pos.Line,
		Column:    pos.Column,
		EndLine:   end.Line,
		EndColumn: end.Column,
		LogCalls:  []LogCallInfo{},
	}
	
	// 分析函数体中的日志调用
	if funcDecl.Body != nil {
		ast.Inspect(funcDecl.Body, func(n ast.Node) bool {
			if callExpr, ok := n.(*ast.CallExpr); ok {
				if logCall := ga.analyzeCallExpr(callExpr); logCall != nil {
					funcInfo.LogCalls = append(funcInfo.LogCalls, *logCall)
					funcInfo.HasLogging = true
				}
			}
			return true
		})
	}
	
	return funcInfo
}

// analyzeCallExpr 分析函数调用表达式
func (ga *GoAnalyzer) analyzeCallExpr(callExpr *ast.CallExpr) *LogCallInfo {
	pos := ga.fileSet.Position(callExpr.Pos())
	
	// 获取函数调用名称
	funcName := ga.getFunctionName(callExpr.Fun)
	if funcName == "" {
		return nil
	}
	
	// 检查是否为日志函数
	for libName, lib := range ga.logFuncs {
		if level, exists := lib.Functions[funcName]; exists {
			return &LogCallInfo{
				FunctionName: funcName,
				Library:      libName,
				Level:        level,
				Line:         pos.Line,
				Column:       pos.Column,
			}
		}
	}
	
	return nil
}

// getFunctionName 获取函数调用名称
func (ga *GoAnalyzer) getFunctionName(expr ast.Expr) string {
	switch e := expr.(type) {
	case *ast.Ident:
		return e.Name
	case *ast.SelectorExpr:
		if pkg, ok := e.X.(*ast.Ident); ok {
			return pkg.Name + "." + e.Sel.Name
		}
	}
	return ""
}

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintf(os.Stderr, "用法: %s <request.json>\n", os.Args[0])
		os.Exit(1)
	}
	
	requestFile := os.Args[1]
	
	// 读取请求文件
	requestData, err := ioutil.ReadFile(requestFile)
	if err != nil {
		fmt.Fprintf(os.Stderr, "读取请求文件失败: %v\n", err)
		os.Exit(1)
	}
	
	// 解析请求
	var request AnalysisRequest
	if err := json.Unmarshal(requestData, &request); err != nil {
		fmt.Fprintf(os.Stderr, "解析请求失败: %v\n", err)
		os.Exit(1)
	}
	
	// 创建分析器
	analyzer := NewGoAnalyzer(request.Config)
	
	// 分析文件
	result, err := analyzer.AnalyzeFile(request.FilePath)
	if err != nil {
		result = &AnalysisResult{
			Success:  false,
			Error:    err.Error(),
			FilePath: request.FilePath,
		}
	}
	
	// 输出结果
	resultData, err := json.MarshalIndent(result, "", "  ")
	if err != nil {
		fmt.Fprintf(os.Stderr, "序列化结果失败: %v\n", err)
		os.Exit(1)
	}
	
	fmt.Print(string(resultData))
} 