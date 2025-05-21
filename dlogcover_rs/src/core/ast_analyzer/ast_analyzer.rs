use std::path::{Path, PathBuf};
use std::collections::HashMap;
use std::ffi::{CString, CStr};
use std::os::raw::c_char;

use crate::config::Config;
use crate::source_manager::SourceFile;
use clang_sys::*;
use log::{debug, error, info, warn};
use serde::Serialize; // Added Serialize

// --- AST Information Structs ---

#[derive(Debug, Clone, Serialize)] // Added Serialize
#[allow(dead_code)]
pub struct SourceLocation {
    pub file_path: PathBuf,
    pub line: u32,
    pub column: u32,
}

#[derive(Debug, Clone, Serialize)] // Added Serialize
#[allow(dead_code)]
pub struct FunctionInfo {
    pub name: String,
    pub qualified_name: String, 
    pub start_location: SourceLocation,
    pub end_location: SourceLocation,
    pub is_method: bool,
    pub class_name: Option<String>, 
    pub return_type: String,
    pub parameters: Vec<String>, 
    pub has_body: bool,         
}

#[derive(Debug, Clone, Serialize)] // Added Serialize
#[allow(dead_code)]
pub struct BranchInfo {
    pub parent_function_qualified_name: String, 
    pub branch_type: String, 
    pub start_location: SourceLocation,
    pub end_location: SourceLocation,
    pub condition_expression: Option<String>, 
}

#[derive(Debug, Clone, Serialize)] // Added Serialize
#[allow(dead_code)]
pub struct ExceptionInfo {
    pub parent_function_qualified_name: String, 
    pub exception_type: String, 
    pub caught_type: Option<String>, 
    pub start_location: SourceLocation,
    pub end_location: SourceLocation,
}

#[derive(Debug, Clone, Serialize)] // Added Serialize
#[allow(dead_code)]
pub struct FileAstInfo {
    pub file_path: PathBuf, 
    pub functions: Vec<FunctionInfo>,
    pub branches: Vec<BranchInfo>,
    pub exceptions: Vec<ExceptionInfo>,
}

impl FileAstInfo {
    #[allow(dead_code)]
    pub fn new(file_path: PathBuf) -> Self {
        FileAstInfo {
            file_path,
            functions: Vec::new(),
            branches: Vec::new(),
            exceptions: Vec::new(),
        }
    }
}

// --- AstAnalyzer Struct ---
#[allow(dead_code)]
pub struct AstAnalyzer<'a> {
    config: &'a Config,
    analysis_results: HashMap<PathBuf, FileAstInfo>,
}

// --- Helper Functions ---
#[allow(dead_code)]
fn cxstring_to_string(cx_string: CXString) -> String {
    if cx_string.data.is_null() { return String::new(); }
    let c_str = unsafe { CStr::from_ptr(clang_getCString(cx_string)) };
    let rust_str = c_str.to_string_lossy().into_owned();
    unsafe { clang_disposeString(cx_string) };
    rust_str
}

#[allow(dead_code)]
fn get_source_location_from_clang(cx_location: CXSourceLocation) -> SourceLocation {
    let mut file = std::ptr::null_mut();
    let mut line = 0;
    let mut column = 0;
    let mut offset = 0; 
    unsafe {
        clang_getSpellingLocation(cx_location, &mut file, &mut line, &mut column, &mut offset);
    }
    let file_path_str = if file.is_null() {
        String::from("<unknown_file>")
    } else {
        cxstring_to_string(unsafe { clang_getFileName(file) })
    };
    SourceLocation {
        file_path: PathBuf::from(file_path_str),
        line,
        column,
    }
}

#[allow(dead_code)]
struct VisitorContext<'v_data> {
    ast_info: &'v_data mut FileAstInfo,
    current_file_path: &'v_data Path, 
    current_function_qname: Option<String>, 
    current_class_name: Option<String>,  
}


// --- AstAnalyzer Implementation ---
#[allow(dead_code)]
impl<'a> AstAnalyzer<'a> {
    pub fn new(config: &'a Config) -> Self {
        debug!("Initializing AstAnalyzer...");
        AstAnalyzer {
            config,
            analysis_results: HashMap::new(),
        }
    }

    pub fn analyze_files(&mut self, source_files: &[SourceFile]) -> Result<(), String> {
        info!("Starting AST analysis for {} files...", source_files.len());
        for source_file in source_files {
            debug!("Analyzing file: {}", source_file.absolute_path.display());
            match self.parse_and_visit_file(&source_file.absolute_path) {
                Ok(file_ast_info) => {
                    info!("Successfully analyzed AST for: {}", source_file.absolute_path.display());
                    self.analysis_results.insert(source_file.absolute_path.clone(), file_ast_info);
                }
                Err(e) => {
                    error!("Failed to analyze AST for {}: {}", source_file.absolute_path.display(), e);
                }
            }
        }
        info!("AST analysis finished. Results collected for {} files.", self.analysis_results.len());
        Ok(())
    }

    fn parse_and_visit_file(&self, file_path: &Path) -> Result<FileAstInfo, String> {
        info!("Parsing and visiting file: {}", file_path.display());
        let mut ast_info = FileAstInfo::new(file_path.to_path_buf());

        unsafe {
            let index = clang_createIndex(0, 0);
            if index.is_null() { return Err("Failed to create Clang Index".to_string()); }

            let c_file_path = CString::new(file_path.to_string_lossy().as_bytes())
                .map_err(|e| format!("Failed to convert file path to CString: {}", e))?;
            
            let arg1_std_cpp17 = CString::new("-std=c++17").unwrap();
            let arg2_xcpp = CString::new("-xc++").unwrap();
            let args_vec: Vec<*const c_char> = vec![
                arg1_std_cpp17.as_ptr(),
                arg2_xcpp.as_ptr(),
            ];
            
            let tu = clang_parseTranslationUnit(
                index, c_file_path.as_ptr(), args_vec.as_ptr(), args_vec.len() as i32,
                std::ptr::null_mut(), 0, CXTranslationUnit_None,
            );

            if tu.is_null() {
                clang_disposeIndex(index);
                return Err(format!("Failed to create TranslationUnit for {}", file_path.display()));
            }

            let num_diagnostics = clang_getNumDiagnostics(tu);
            for i in 0..num_diagnostics {
                let diagnostic = clang_getDiagnostic(tu, i);
                let diag_string = cxstring_to_string(clang_formatDiagnostic(diagnostic, clang_defaultDiagnosticDisplayOptions()));
                let severity = clang_getDiagnosticSeverity(diagnostic);
                if severity == CXDiagnostic_Error || severity == CXDiagnostic_Fatal {
                    error!("Clang [Error/Fatal]: {}", diag_string);
                } else if severity == CXDiagnostic_Warning {
                    warn!("Clang [Warning]: {}", diag_string);
                }
                clang_disposeDiagnostic(diagnostic);
            }
            if (0..num_diagnostics).any(|i| {
                let d = clang_getDiagnostic(tu, i); 
                let s = clang_getDiagnosticSeverity(d); 
                clang_disposeDiagnostic(d); 
                s == CXDiagnostic_Error || s == CXDiagnostic_Fatal
            }) {
                 error!("Fatal parsing errors in {}. Aborting analysis for this file.", file_path.display());
                 clang_disposeTranslationUnit(tu);
                 clang_disposeIndex(index);
                 return Err(format!("Fatal parsing errors in {}", file_path.display()));
            }

            let cursor = clang_getTranslationUnitCursor(tu);
            let mut visitor_context = VisitorContext {
                ast_info: &mut ast_info,
                current_file_path: file_path,
                current_function_qname: None,
                current_class_name: None,
            };

            clang_visitChildren(
                cursor,
                visit_cursor_recursive,
                &mut visitor_context as *mut _ as *mut std::ffi::c_void,
            );

            clang_disposeTranslationUnit(tu);
            clang_disposeIndex(index);
        }
        
        info!("Finished visiting AST for {}. Functions: {}, Branches: {}, Exceptions: {}", 
              file_path.display(), ast_info.functions.len(), ast_info.branches.len(), ast_info.exceptions.len());
        Ok(ast_info)
    }

    pub fn get_analysis_results(&self) -> &HashMap<PathBuf, FileAstInfo> {
        &self.analysis_results
    }
}

// --- AST Visitor Callback and Helpers ---

#[allow(dead_code)]
fn get_cursor_source_text(cursor: CXCursor, tu: CXTranslationUnit) -> Option<String> {
    let extent = unsafe { clang_getCursorExtent(cursor) };
    let mut tokens_ptr = std::ptr::null_mut();
    let mut num_tokens = 0;
    unsafe { clang_tokenize(tu, extent, &mut tokens_ptr, &mut num_tokens) };

    if tokens_ptr.is_null() || num_tokens == 0 {
        return None;
    }

    let tokens = unsafe { std::slice::from_raw_parts(tokens_ptr, num_tokens as usize) };
    let mut text = String::new();
    for (i, token) in tokens.iter().enumerate() {
        text.push_str(&cxstring_to_string(unsafe { clang_getTokenSpelling(tu, *token) }));
        if i < tokens.len() - 1 {
            text.push(' '); 
        }
    }
    unsafe { clang_disposeTokens(tu, tokens_ptr, num_tokens) };
    Some(text)
}

#[allow(non_upper_case_globals)] 
extern "C" fn visit_cursor_recursive(
    cursor: CXCursor,
    _parent: CXCursor, 
    client_data: *mut std::ffi::c_void,
) -> CXChildVisitResult {
    let data = unsafe { &mut *(client_data as *mut VisitorContext) };
    let location = unsafe { clang_getCursorLocation(cursor) };

    if unsafe { clang_Location_isFromMainFile(location) } == 0 {
        return CXChildVisit_Recurse; 
    }
    
    let extent = unsafe { clang_getCursorExtent(cursor) };
    let start_cx_loc = unsafe { clang_getRangeStart(extent) };
    let end_cx_loc = unsafe { clang_getRangeEnd(extent) };
    let start_loc = get_source_location_from_clang(start_cx_loc);
    let end_loc = get_source_location_from_clang(end_cx_loc);

    let kind = unsafe { clang_getCursorKind(cursor) };

    let original_function_qname = data.current_function_qname.clone();
    let original_class_name = data.current_class_name.clone();

    match kind {
        CXCursor_Namespace => {}
        CXCursor_ClassDecl | CXCursor_StructDecl => {
            let name = cxstring_to_string(unsafe { clang_getCursorSpelling(cursor) });
            data.current_class_name = Some(name);
        }
        CXCursor_FunctionDecl | CXCursor_CXXMethod => {
            if unsafe { clang_isCursorDefinition(cursor) } != 0 {
                let name = cxstring_to_string(unsafe { clang_getCursorSpelling(cursor) });
                let usr = cxstring_to_string(unsafe { clang_getCursorUSR(cursor) });
                let qualified_name = if usr.is_empty() { name.clone() } else { usr };

                let cursor_type = unsafe { clang_getCursorType(cursor) };
                let result_type = unsafe { clang_getResultType(cursor_type) };
                let return_type_str = cxstring_to_string(unsafe { clang_getTypeSpelling(result_type) });

                let mut parameters = Vec::new();
                let num_args = unsafe { clang_Cursor_getNumArguments(cursor) };
                for i in 0..num_args {
                    let arg_cursor = unsafe { clang_Cursor_getArgument(cursor, i as u32) };
                    let arg_name = cxstring_to_string(unsafe { clang_getCursorSpelling(arg_cursor) });
                    let arg_type_cursor = unsafe { clang_getCursorType(arg_cursor) };
                    let arg_type_str = cxstring_to_string(unsafe { clang_getTypeSpelling(arg_type_cursor) });
                    if arg_name.is_empty() {
                        parameters.push(arg_type_str);
                    } else {
                        parameters.push(format!("{}: {}", arg_name, arg_type_str));
                    }
                }
                
                let func_info = FunctionInfo {
                    name,
                    qualified_name: qualified_name.clone(),
                    start_location: start_loc,
                    end_location: end_loc,
                    is_method: kind == CXCursor_CXXMethod,
                    class_name: if kind == CXCursor_CXXMethod { data.current_class_name.clone() } else { None },
                    return_type: return_type_str,
                    parameters,
                    has_body: true,
                };
                data.ast_info.functions.push(func_info);
                data.current_function_qname = Some(qualified_name);
            }
        }
        CXCursor_IfStmt => {
            if let Some(func_qname) = &data.current_function_qname {
                data.ast_info.branches.push(BranchInfo {
                    parent_function_qualified_name: func_qname.clone(),
                    branch_type: "if".to_string(),
                    start_location: start_loc,
                    end_location: end_loc, 
                    condition_expression: None, 
                });
            }
        }
        CXCursor_SwitchStmt => {
             if let Some(func_qname) = &data.current_function_qname {
                data.ast_info.branches.push(BranchInfo {
                    parent_function_qualified_name: func_qname.clone(),
                    branch_type: "switch".to_string(),
                    start_location: start_loc,
                    end_location: end_loc,
                    condition_expression: None, 
                });
            }
        }
        CXCursor_CaseStmt | CXCursor_DefaultStmt => {
            if let Some(func_qname) = &data.current_function_qname {
                data.ast_info.branches.push(BranchInfo {
                    parent_function_qualified_name: func_qname.clone(),
                    branch_type: if kind == CXCursor_CaseStmt { "case" } else { "default" }.to_string(),
                    start_location: start_loc,
                    end_location: end_loc,
                    condition_expression: None, 
                });
            }
        }
        CXCursor_ForStmt | CXCursor_WhileStmt | CXCursor_DoStmt => {
            if let Some(func_qname) = &data.current_function_qname {
                let branch_type = match kind {
                    CXCursor_ForStmt => "for",
                    CXCursor_WhileStmt => "while",
                    CXCursor_DoStmt => "do_while",
                    _ => unreachable!(),
                }.to_string();
                data.ast_info.branches.push(BranchInfo {
                    parent_function_qualified_name: func_qname.clone(),
                    branch_type,
                    start_location: start_loc,
                    end_location: end_loc,
                    condition_expression: None, 
                });
            }
        }
        CXCursor_CXXTryStmt => {
             if let Some(func_qname) = &data.current_function_qname {
                data.ast_info.exceptions.push(ExceptionInfo {
                    parent_function_qualified_name: func_qname.clone(),
                    exception_type: "try".to_string(),
                    caught_type: None,
                    start_location: start_loc,
                    end_location: end_loc,
                });
            }
        }
        CXCursor_CXXCatchStmt => {
            if let Some(func_qname) = &data.current_function_qname {
                let caught_type_cursor = unsafe { clang_getCursorType(cursor) }; 
                let caught_type_str = cxstring_to_string(unsafe { clang_getTypeSpelling(caught_type_cursor) });
                data.ast_info.exceptions.push(ExceptionInfo {
                    parent_function_qualified_name: func_qname.clone(),
                    exception_type: "catch".to_string(),
                    caught_type: if caught_type_str.is_empty() || caught_type_str == "..." { None } else { Some(caught_type_str) },
                    start_location: start_loc,
                    end_location: end_loc,
                });
            }
        }
        _ => {}
    }

    unsafe {
        clang_visitChildren(
            cursor,
            visit_cursor_recursive,
            client_data,
        );
    }
    
    data.current_function_qname = original_function_qname;
    data.current_class_name = original_class_name;

    CXChildVisit_Continue 
}
