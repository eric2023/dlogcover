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

// AnalysisResult 分析结果结构 - 扩展支持分支和错误处理分析
type AnalysisResult struct {
	Success         bool                  `json:"success"`
	Error           string                `json:"error,omitempty"`
	FilePath        string                `json:"file_path"`
	Functions       []FunctionInfo        `json:"functions"`
	LogCalls        []LogCallInfo         `json:"log_calls"`
	
	// 新增：分支分析结果
	Branches        []BranchInfo          `json:"branches"`
	
	// 新增：错误处理分析结果  
	ErrorHandlings  []ErrorHandlingInfo   `json:"error_handlings"`
	
	// 增强的统计信息
	Statistics      EnhancedStatistics    `json:"statistics"`
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

// 新增：分支信息结构
type BranchInfo struct {
	Type        string        `json:"type"`         // "if", "switch", "for", "range", "type_switch"
	Line        int           `json:"line"`
	Column      int           `json:"column"`
	EndLine     int           `json:"end_line"`
	EndColumn   int           `json:"end_column"`
	HasLogging  bool          `json:"has_logging"`
	Branches    []BranchPath  `json:"branches"`     // 各个分支路径
	LogCalls    []LogCallInfo `json:"log_calls"`
	Context     string        `json:"context"`      // 分支的上下文代码片段
}

// 新增：分支路径结构
type BranchPath struct {
	PathType    string        `json:"path_type"`    // "then", "else", "case", "default", "body"
	CaseValue   string        `json:"case_value,omitempty"` // switch case的值
	HasLogging  bool          `json:"has_logging"`
	LogCalls    []LogCallInfo `json:"log_calls"`
	Line        int           `json:"line"`
	Column      int           `json:"column"`
}

// 新增：错误处理信息结构
type ErrorHandlingInfo struct {
	Type        string        `json:"type"`         // "error_check", "panic", "recover"
	Pattern     string        `json:"pattern"`      // "if_err_nil", "panic_call", "defer_recover"
	Line        int           `json:"line"`
	Column      int           `json:"column"`
	HasLogging  bool          `json:"has_logging"`
	LogCalls    []LogCallInfo `json:"log_calls"`
	Context     string        `json:"context"`      // 错误处理的上下文代码
	ErrorVar    string        `json:"error_var,omitempty"` // 错误变量名
}

// 增强的统计信息结构
type EnhancedStatistics struct {
	TotalFunctions      int            `json:"total_functions"`
	TotalLogCalls       int            `json:"total_log_calls"`
	
	// 新增：分支统计
	TotalBranches       int            `json:"total_branches"`
	CoveredBranches     int            `json:"covered_branches"`
	BranchCoverage      float64        `json:"branch_coverage"`
	
	// 新增：错误处理统计
	TotalErrorHandlings int            `json:"total_error_handlings"`
	CoveredErrorHandlings int          `json:"covered_error_handlings"`
	ErrorHandlingCoverage float64      `json:"error_handling_coverage"`
	
	// 分支类型统计
	BranchTypeStats     map[string]int `json:"branch_type_stats"`
	
	// 错误处理类型统计
	ErrorTypeStats      map[string]int `json:"error_type_stats"`
	
	LibraryStats        map[string]int `json:"library_stats"`
}

// 原有的Statistics结构保持兼容性
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
			var fullFuncName string
			if strings.HasPrefix(fn, "log.") {
				fullFuncName = fn
			} else {
				fullFuncName = "log." + fn
			}
			stdLib.Functions[fullFuncName] = level
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

// AnalyzeFile 分析Go文件 - 扩展支持分支和错误处理分析
func (ga *GoAnalyzer) AnalyzeFile(filePath string) (*AnalysisResult, error) {
	result := &AnalysisResult{
		FilePath:       filePath,
		Functions:      []FunctionInfo{},
		LogCalls:       []LogCallInfo{},
		Branches:       []BranchInfo{},          // 新增：初始化分支分析结果
		ErrorHandlings: []ErrorHandlingInfo{},   // 新增：初始化错误处理分析结果
		Statistics: EnhancedStatistics{          // 使用增强的统计信息
			LibraryStats:      make(map[string]int),
			BranchTypeStats:   make(map[string]int),
			ErrorTypeStats:    make(map[string]int),
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
	
	// 遍历AST节点进行多种类型的分析
	ast.Inspect(file, func(n ast.Node) bool {
		switch node := n.(type) {
		case *ast.FuncDecl:
			// 现有：函数分析
			if node.Name != nil {
				funcInfo := ga.analyzeFunctionDecl(node)
				result.Functions = append(result.Functions, funcInfo)
				result.LogCalls = append(result.LogCalls, funcInfo.LogCalls...)
			}
			
		// 新增：分支结构分析
		case *ast.IfStmt:
			branchInfo := ga.analyzeIfStatement(node)
			result.Branches = append(result.Branches, branchInfo)
			if branchInfo.HasLogging {
				result.LogCalls = append(result.LogCalls, branchInfo.LogCalls...)
			}
			
		case *ast.SwitchStmt:
			branchInfo := ga.analyzeSwitchStatement(node)
			result.Branches = append(result.Branches, branchInfo)
			if branchInfo.HasLogging {
				result.LogCalls = append(result.LogCalls, branchInfo.LogCalls...)
			}
			
		case *ast.TypeSwitchStmt:
			branchInfo := ga.analyzeTypeSwitchStatement(node)
			result.Branches = append(result.Branches, branchInfo)
			if branchInfo.HasLogging {
				result.LogCalls = append(result.LogCalls, branchInfo.LogCalls...)
			}
			
		case *ast.ForStmt:
			branchInfo := ga.analyzeForStatement(node)
			result.Branches = append(result.Branches, branchInfo)
			if branchInfo.HasLogging {
				result.LogCalls = append(result.LogCalls, branchInfo.LogCalls...)
			}
			
		case *ast.RangeStmt:
			branchInfo := ga.analyzeRangeStatement(node)
			result.Branches = append(result.Branches, branchInfo)
			if branchInfo.HasLogging {
				result.LogCalls = append(result.LogCalls, branchInfo.LogCalls...)
			}
		}
		return true
	})
	
	// 新增：错误处理分析（需要单独遍历，因为错误处理模式可能跨多个节点）
	errorHandlings := ga.analyzeErrorHandling(file)
	result.ErrorHandlings = append(result.ErrorHandlings, errorHandlings...)
	for _, errorHandling := range errorHandlings {
		if errorHandling.HasLogging {
			result.LogCalls = append(result.LogCalls, errorHandling.LogCalls...)
		}
	}
	
	// 计算增强的统计信息
	ga.calculateEnhancedStatistics(result)
	
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

// ===== 分支分析相关函数 =====

// analyzeIfStatement 分析if语句
func (ga *GoAnalyzer) analyzeIfStatement(ifStmt *ast.IfStmt) BranchInfo {
	pos := ga.fileSet.Position(ifStmt.Pos())
	end := ga.fileSet.Position(ifStmt.End())
	
	branchInfo := BranchInfo{
		Type:      "if",
		Line:      pos.Line,
		Column:    pos.Column,
		EndLine:   end.Line,
		EndColumn: end.Column,
		Branches:  []BranchPath{},
		LogCalls:  []LogCallInfo{},
		Context:   ga.getNodeText(ifStmt),
	}
	
	// 分析then分支
	if ifStmt.Body != nil {
		thenPath := ga.analyzeBranchPath("then", ifStmt.Body)
		branchInfo.Branches = append(branchInfo.Branches, thenPath)
		if thenPath.HasLogging {
			branchInfo.HasLogging = true
			branchInfo.LogCalls = append(branchInfo.LogCalls, thenPath.LogCalls...)
		}
	}
	
	// 分析else分支
	if ifStmt.Else != nil {
		elsePath := ga.analyzeBranchPath("else", ifStmt.Else)
		branchInfo.Branches = append(branchInfo.Branches, elsePath)
		if elsePath.HasLogging {
			branchInfo.HasLogging = true
			branchInfo.LogCalls = append(branchInfo.LogCalls, elsePath.LogCalls...)
		}
	}
	
	return branchInfo
}

// analyzeSwitchStatement 分析switch语句
func (ga *GoAnalyzer) analyzeSwitchStatement(switchStmt *ast.SwitchStmt) BranchInfo {
	pos := ga.fileSet.Position(switchStmt.Pos())
	end := ga.fileSet.Position(switchStmt.End())
	
	branchInfo := BranchInfo{
		Type:      "switch",
		Line:      pos.Line,
		Column:    pos.Column,
		EndLine:   end.Line,
		EndColumn: end.Column,
		Branches:  []BranchPath{},
		LogCalls:  []LogCallInfo{},
		Context:   ga.getNodeText(switchStmt),
	}
	
	// 分析switch body中的case语句
	if switchStmt.Body != nil {
		for _, stmt := range switchStmt.Body.List {
			if caseClause, ok := stmt.(*ast.CaseClause); ok {
				casePath := ga.analyzeCaseClause(caseClause)
				branchInfo.Branches = append(branchInfo.Branches, casePath)
				if casePath.HasLogging {
					branchInfo.HasLogging = true
					branchInfo.LogCalls = append(branchInfo.LogCalls, casePath.LogCalls...)
				}
			}
		}
	}
	
	return branchInfo
}

// analyzeTypeSwitchStatement 分析类型switch语句
func (ga *GoAnalyzer) analyzeTypeSwitchStatement(typeSwitchStmt *ast.TypeSwitchStmt) BranchInfo {
	pos := ga.fileSet.Position(typeSwitchStmt.Pos())
	end := ga.fileSet.Position(typeSwitchStmt.End())
	
	branchInfo := BranchInfo{
		Type:      "type_switch",
		Line:      pos.Line,
		Column:    pos.Column,
		EndLine:   end.Line,
		EndColumn: end.Column,
		Branches:  []BranchPath{},
		LogCalls:  []LogCallInfo{},
		Context:   ga.getNodeText(typeSwitchStmt),
	}
	
	// 分析type switch body中的case语句
	if typeSwitchStmt.Body != nil {
		for _, stmt := range typeSwitchStmt.Body.List {
			if caseClause, ok := stmt.(*ast.CaseClause); ok {
				casePath := ga.analyzeCaseClause(caseClause)
				casePath.PathType = "type_case" // 标记为类型case
				branchInfo.Branches = append(branchInfo.Branches, casePath)
				if casePath.HasLogging {
					branchInfo.HasLogging = true
					branchInfo.LogCalls = append(branchInfo.LogCalls, casePath.LogCalls...)
				}
			}
		}
	}
	
	return branchInfo
}

// analyzeForStatement 分析for循环语句
func (ga *GoAnalyzer) analyzeForStatement(forStmt *ast.ForStmt) BranchInfo {
	pos := ga.fileSet.Position(forStmt.Pos())
	end := ga.fileSet.Position(forStmt.End())
	
	branchInfo := BranchInfo{
		Type:      "for",
		Line:      pos.Line,
		Column:    pos.Column,
		EndLine:   end.Line,
		EndColumn: end.Column,
		Branches:  []BranchPath{},
		LogCalls:  []LogCallInfo{},
		Context:   ga.getNodeText(forStmt),
	}
	
	// 分析for循环体
	if forStmt.Body != nil {
		bodyPath := ga.analyzeBranchPath("body", forStmt.Body)
		branchInfo.Branches = append(branchInfo.Branches, bodyPath)
		if bodyPath.HasLogging {
			branchInfo.HasLogging = true
			branchInfo.LogCalls = append(branchInfo.LogCalls, bodyPath.LogCalls...)
		}
	}
	
	return branchInfo
}

// analyzeRangeStatement 分析range循环语句
func (ga *GoAnalyzer) analyzeRangeStatement(rangeStmt *ast.RangeStmt) BranchInfo {
	pos := ga.fileSet.Position(rangeStmt.Pos())
	end := ga.fileSet.Position(rangeStmt.End())
	
	branchInfo := BranchInfo{
		Type:      "range",
		Line:      pos.Line,
		Column:    pos.Column,
		EndLine:   end.Line,
		EndColumn: end.Column,
		Branches:  []BranchPath{},
		LogCalls:  []LogCallInfo{},
		Context:   ga.getNodeText(rangeStmt),
	}
	
	// 分析range循环体
	if rangeStmt.Body != nil {
		bodyPath := ga.analyzeBranchPath("body", rangeStmt.Body)
		branchInfo.Branches = append(branchInfo.Branches, bodyPath)
		if bodyPath.HasLogging {
			branchInfo.HasLogging = true
			branchInfo.LogCalls = append(branchInfo.LogCalls, bodyPath.LogCalls...)
		}
	}
	
	return branchInfo
}

// analyzeBranchPath 分析分支路径中的日志调用
func (ga *GoAnalyzer) analyzeBranchPath(pathType string, stmt ast.Stmt) BranchPath {
	pos := ga.fileSet.Position(stmt.Pos())
	
	path := BranchPath{
		PathType:   pathType,
		HasLogging: false,
		LogCalls:   []LogCallInfo{},
		Line:       pos.Line,
		Column:     pos.Column,
	}
	
	// 深度遍历分支内的所有语句，寻找日志调用
	ast.Inspect(stmt, func(n ast.Node) bool {
		if callExpr, ok := n.(*ast.CallExpr); ok {
			if logCall := ga.analyzeCallExpr(callExpr); logCall != nil {
				path.LogCalls = append(path.LogCalls, *logCall)
				path.HasLogging = true
			}
		}
		return true
	})
	
	return path
}

// analyzeCaseClause 分析case子句
func (ga *GoAnalyzer) analyzeCaseClause(caseClause *ast.CaseClause) BranchPath {
	pos := ga.fileSet.Position(caseClause.Pos())
	
	pathType := "case"
	caseValue := ""
	
	// 判断是否为default case
	if caseClause.List == nil {
		pathType = "default"
	} else if len(caseClause.List) > 0 {
		// 获取case值的文本表示
		caseValue = ga.getNodeText(caseClause.List[0])
	}
	
	path := BranchPath{
		PathType:   pathType,
		CaseValue:  caseValue,
		HasLogging: false,
		LogCalls:   []LogCallInfo{},
		Line:       pos.Line,
		Column:     pos.Column,
	}
	
	// 分析case体中的日志调用
	for _, stmt := range caseClause.Body {
		ast.Inspect(stmt, func(n ast.Node) bool {
			if callExpr, ok := n.(*ast.CallExpr); ok {
				if logCall := ga.analyzeCallExpr(callExpr); logCall != nil {
					path.LogCalls = append(path.LogCalls, *logCall)
					path.HasLogging = true
				}
			}
			return true
		})
	}
	
	return path
}

// ===== 错误处理分析相关函数 =====

// analyzeErrorHandling 分析错误处理模式
func (ga *GoAnalyzer) analyzeErrorHandling(file *ast.File) []ErrorHandlingInfo {
	var errorHandlings []ErrorHandlingInfo
	
	ast.Inspect(file, func(n ast.Node) bool {
		switch node := n.(type) {
		case *ast.IfStmt:
			// 检查是否为错误处理模式
			if errorInfo := ga.checkErrorHandlingPattern(node); errorInfo != nil {
				errorHandlings = append(errorHandlings, *errorInfo)
			}
			
		case *ast.CallExpr:
			// 检查panic调用
			if ga.isPanicCall(node) {
				panicInfo := ga.analyzePanicCall(node)
				errorHandlings = append(errorHandlings, panicInfo)
			}
			
			// 检查recover调用
			if ga.isRecoverCall(node) {
				recoverInfo := ga.analyzeRecoverCall(node)
				errorHandlings = append(errorHandlings, recoverInfo)
			}
		}
		return true
	})
	
	return errorHandlings
}

// checkErrorHandlingPattern 检查错误处理模式
func (ga *GoAnalyzer) checkErrorHandlingPattern(ifStmt *ast.IfStmt) *ErrorHandlingInfo {
	// 检查是否为 "if err != nil" 模式
	if ga.isErrorCheckCondition(ifStmt.Cond) {
		pos := ga.fileSet.Position(ifStmt.Pos())
		
		errorInfo := &ErrorHandlingInfo{
			Type:       "error_check",
			Pattern:    "if_err_nil",
			Line:       pos.Line,
			Column:     pos.Column,
			HasLogging: false,
			LogCalls:   []LogCallInfo{},
			Context:    ga.getNodeText(ifStmt),
			ErrorVar:   ga.extractErrorVar(ifStmt.Cond),
		}
		
		// 分析错误处理分支中的日志调用
		if ifStmt.Body != nil {
			ast.Inspect(ifStmt.Body, func(n ast.Node) bool {
				if callExpr, ok := n.(*ast.CallExpr); ok {
					if logCall := ga.analyzeCallExpr(callExpr); logCall != nil {
						errorInfo.LogCalls = append(errorInfo.LogCalls, *logCall)
						errorInfo.HasLogging = true
					}
				}
				return true
			})
		}
		
		return errorInfo
	}
	
	return nil
}

// isErrorCheckCondition 判断是否为错误检查条件
func (ga *GoAnalyzer) isErrorCheckCondition(expr ast.Expr) bool {
	if binaryExpr, ok := expr.(*ast.BinaryExpr); ok {
		// 检查 "err != nil" 模式
		if binaryExpr.Op.String() == "!=" {
			// 检查左侧是否为错误变量
			if ident, ok := binaryExpr.X.(*ast.Ident); ok {
				if strings.Contains(strings.ToLower(ident.Name), "err") {
					// 检查右侧是否为nil
					if ident2, ok := binaryExpr.Y.(*ast.Ident); ok {
						return ident2.Name == "nil"
					}
				}
			}
		}
	}
	return false
}

// extractErrorVar 提取错误变量名
func (ga *GoAnalyzer) extractErrorVar(expr ast.Expr) string {
	if binaryExpr, ok := expr.(*ast.BinaryExpr); ok {
		if ident, ok := binaryExpr.X.(*ast.Ident); ok {
			return ident.Name
		}
	}
	return ""
}

// isPanicCall 判断是否为panic调用
func (ga *GoAnalyzer) isPanicCall(callExpr *ast.CallExpr) bool {
	if ident, ok := callExpr.Fun.(*ast.Ident); ok {
		return ident.Name == "panic"
	}
	return false
}

// isRecoverCall 判断是否为recover调用
func (ga *GoAnalyzer) isRecoverCall(callExpr *ast.CallExpr) bool {
	if ident, ok := callExpr.Fun.(*ast.Ident); ok {
		return ident.Name == "recover"
	}
	return false
}

// analyzePanicCall 分析panic调用
func (ga *GoAnalyzer) analyzePanicCall(callExpr *ast.CallExpr) ErrorHandlingInfo {
	pos := ga.fileSet.Position(callExpr.Pos())
	
	return ErrorHandlingInfo{
		Type:       "panic",
		Pattern:    "panic_call",
		Line:       pos.Line,
		Column:     pos.Column,
		HasLogging: false, // panic本身不算日志，但可能在panic前有日志
		LogCalls:   []LogCallInfo{},
		Context:    ga.getNodeText(callExpr),
	}
}

// analyzeRecoverCall 分析recover调用
func (ga *GoAnalyzer) analyzeRecoverCall(callExpr *ast.CallExpr) ErrorHandlingInfo {
	pos := ga.fileSet.Position(callExpr.Pos())
	
	return ErrorHandlingInfo{
		Type:       "recover",
		Pattern:    "defer_recover",
		Line:       pos.Line,
		Column:     pos.Column,
		HasLogging: false, // recover本身不算日志，但可能在recover后有日志
		LogCalls:   []LogCallInfo{},
		Context:    ga.getNodeText(callExpr),
	}
}

// ===== 辅助函数 =====

// getNodeText 获取AST节点的源码文本
func (ga *GoAnalyzer) getNodeText(node ast.Node) string {
	pos := ga.fileSet.Position(node.Pos())
	end := ga.fileSet.Position(node.End())
	
	// 简化实现：返回位置信息作为上下文
	// 在实际实现中，可以通过文件内容和位置信息提取真实的源码文本
	return fmt.Sprintf("Line %d-%d", pos.Line, end.Line)
}

// calculateEnhancedStatistics 计算增强的统计信息
func (ga *GoAnalyzer) calculateEnhancedStatistics(result *AnalysisResult) {
	// 基础统计
	result.Statistics.TotalFunctions = len(result.Functions)
	result.Statistics.TotalLogCalls = len(result.LogCalls)
	
	// 分支统计
	result.Statistics.TotalBranches = len(result.Branches)
	coveredBranches := 0
	for _, branch := range result.Branches {
		if branch.HasLogging {
			coveredBranches++
		}
		// 统计分支类型
		result.Statistics.BranchTypeStats[branch.Type]++
	}
	result.Statistics.CoveredBranches = coveredBranches
	
	// 计算分支覆盖率
	if result.Statistics.TotalBranches > 0 {
		result.Statistics.BranchCoverage = float64(coveredBranches) / float64(result.Statistics.TotalBranches)
	}
	
	// 错误处理统计
	result.Statistics.TotalErrorHandlings = len(result.ErrorHandlings)
	coveredErrorHandlings := 0
	for _, errorHandling := range result.ErrorHandlings {
		if errorHandling.HasLogging {
			coveredErrorHandlings++
		}
		// 统计错误处理类型
		result.Statistics.ErrorTypeStats[errorHandling.Type]++
	}
	result.Statistics.CoveredErrorHandlings = coveredErrorHandlings
	
	// 计算错误处理覆盖率
	if result.Statistics.TotalErrorHandlings > 0 {
		result.Statistics.ErrorHandlingCoverage = float64(coveredErrorHandlings) / float64(result.Statistics.TotalErrorHandlings)
	}
	
	// 日志库统计
	for _, logCall := range result.LogCalls {
		result.Statistics.LibraryStats[logCall.Library]++
	}
} 