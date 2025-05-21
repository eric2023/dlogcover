pub mod ast_analyzer;

// Re-export key structs and the analyzer itself for easier access from core module
pub use ast_analyzer::{
    AstAnalyzer, 
    FileAstInfo, 
    FunctionInfo, 
    BranchInfo, 
    ExceptionInfo, 
    SourceLocation
};
