use std::path::{Path, PathBuf};
use std::ffi::{CString, CStr};
use std::os::raw::c_char;

use crate::core::ast_analyzer::SourceLocation as AstSourceLocation;
use crate::config::{Config, QtLogConfig, CustomLogConfig}; // Assuming these are public enough
use crate::core::ast_analyzer::FileAstInfo;
// use crate::utils::file_utils; // Not used directly in this snippet
use clang_sys::*;
use log::{debug, error, info, warn};
use serde::Serialize; // Added Serialize

// --- Log Information Structs and Enums ---

#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize)] // Added Serialize
#[allow(dead_code)]
pub enum LogLevel {
    Debug,
    Info,
    Warning,
    Critical, 
    Fatal,    
    Unknown,
}

#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize)] // Added Serialize
#[allow(dead_code)]
pub enum LogType {
    Qt,
    QtCategory,
    Custom,
    Unknown,
}

#[derive(Debug, Clone, Serialize)] // Added Serialize
#[allow(dead_code)]
pub struct LogCallSite {
    pub function_name: String,
    pub source_location: AstSourceLocation, 
    pub log_level: LogLevel,
    pub log_type: LogType,
    pub parent_function_qualified_name: String,
    pub containing_class_name: Option<String>,
    pub message_arguments: Vec<String>,
}

// --- VisitorContext for LogIdentifier ---
#[allow(dead_code)]
struct LogVisitorContext<'ctx> {
    log_call_sites: &'ctx mut Vec<LogCallSite>,
    config: &'ctx Config,
    current_file_path: &'ctx Path,
    current_parent_function_qname: Option<String>,
    current_parent_class_name: Option<String>,
    // translation_unit: CXTranslationUnit, 
}

// --- Helper Functions ---
#[allow(dead_code)]
fn cxstring_to_string_log(cx_string: CXString) -> String {
    if cx_string.data.is_null() { return String::new(); }
    let c_str = unsafe { CStr::from_ptr(clang_getCString(cx_string)) };
    let rust_str = c_str.to_string_lossy().into_owned();
    unsafe { clang_disposeString(cx_string) };
    rust_str
}

#[allow(dead_code)]
fn get_source_location_log(cx_location: CXSourceLocation) -> AstSourceLocation {
    let mut file = std::ptr::null_mut();
    let mut line = 0;
    let mut column = 0;
    let mut offset = 0;
    unsafe {
        clang_getSpellingLocation(cx_location, &mut file, &mut line, &mut column, &mut offset);
    }
    let file_path_str = if file.is_null() {
        String::from("<unknown_file_log>")
    } else {
        cxstring_to_string_log(unsafe { clang_getFileName(file) })
    };
    AstSourceLocation {
        file_path: PathBuf::from(file_path_str),
        line,
        column,
    }
}


// --- LogIdentifier Struct ---
#[allow(dead_code)]
pub struct LogIdentifier<'a> {
    config: &'a Config,
}

#[allow(dead_code)]
impl<'a> LogIdentifier<'a> {
    pub fn new(config: &'a Config) -> Self {
        debug!("Initializing LogIdentifier...");
        LogIdentifier { config }
    }

    pub fn identify_log_calls_in_file(
        &self,
        file_ast_info: &FileAstInfo, 
        file_content: &str,
    ) -> Result<Vec<LogCallSite>, String> {
        info!("Identifying log calls in file: {}", file_ast_info.file_path.display());
        let mut log_call_sites = Vec::new();

        unsafe {
            let index = clang_createIndex(0, 0);
            if index.is_null() { return Err("LogIdentifier: Failed to create Clang Index".to_string()); }

            let c_file_path_str = file_ast_info.file_path.to_string_lossy();
            let c_file_path = CString::new(c_file_path_str.as_bytes())
                .map_err(|e| format!("LogIdentifier: Failed to convert file path to CString: {}", e))?;

            let c_file_content = CString::new(file_content.as_bytes())
                 .map_err(|e| format!("LogIdentifier: Failed to convert file content to CString: {}", e))?;
            let unsaved_file = CXUnsavedFile {
                Filename: c_file_path.as_ptr(),
                Contents: c_file_content.as_ptr(),
                Length: file_content.len() as std::os::raw::c_ulong,
            };
            
            let arg1_std_cpp17 = CString::new("-std=c++17").unwrap();
            let arg2_xcpp = CString::new("-xc++").unwrap();
            let args_vec: Vec<*const c_char> = vec![
                arg1_std_cpp17.as_ptr(),
                arg2_xcpp.as_ptr(),
            ];

            let tu = clang_parseTranslationUnit(
                index, c_file_path.as_ptr(), 
                args_vec.as_ptr(), args_vec.len() as i32,
                &unsaved_file as *const _ as *mut CXUnsavedFile, 
                1, 
                CXTranslationUnit_None | CXTranslationUnit_DetailedPreprocessingRecord, 
            );

            if tu.is_null() {
                clang_disposeIndex(index);
                return Err(format!("LogIdentifier: Failed to create TranslationUnit for {}", file_ast_info.file_path.display()));
            }
            
            let num_diagnostics = clang_getNumDiagnostics(tu);
            let mut has_fatal_errors = false;
            for i in 0..num_diagnostics {
                let diagnostic = clang_getDiagnostic(tu, i);
                let diag_string = cxstring_to_string_log(clang_formatDiagnostic(diagnostic, clang_defaultDiagnosticDisplayOptions()));
                let severity = clang_getDiagnosticSeverity(diagnostic);
                if severity == CXDiagnostic_Error || severity == CXDiagnostic_Fatal {
                    error!("LogIdentifier Clang [Error/Fatal] for {}: {}", file_ast_info.file_path.display(), diag_string);
                    has_fatal_errors = true;
                } else if severity == CXDiagnostic_Warning {
                    warn!("LogIdentifier Clang [Warning] for {}: {}", file_ast_info.file_path.display(), diag_string);
                }
                clang_disposeDiagnostic(diagnostic);
            }
            if has_fatal_errors {
                 clang_disposeTranslationUnit(tu);
                 clang_disposeIndex(index);
                 return Err(format!("LogIdentifier: Fatal parsing errors in {}", file_ast_info.file_path.display()));
            }


            let cursor = clang_getTranslationUnitCursor(tu);
            let mut visitor_context = LogVisitorContext {
                log_call_sites: &mut log_call_sites,
                config: self.config,
                current_file_path: &file_ast_info.file_path,
                current_parent_function_qname: None,
                current_parent_class_name: None,
                // translation_unit: tu,
            };

            clang_visitChildren(
                cursor,
                visit_log_identifier_cursor,
                &mut visitor_context as *mut _ as *mut std::ffi::c_void,
            );

            clang_disposeTranslationUnit(tu);
            clang_disposeIndex(index);
        }
        
        info!("Found {} log call sites in {}", log_call_sites.len(), file_ast_info.file_path.display());
        Ok(log_call_sites)
    }
}


// --- AST Visitor for LogIdentifier ---
#[allow(non_upper_case_globals)]
extern "C" fn visit_log_identifier_cursor(
    cursor: CXCursor,
    _parent: CXCursor,
    client_data: *mut std::ffi::c_void,
) -> CXChildVisitResult {
    let context = unsafe { &mut *(client_data as *mut LogVisitorContext) };
    let location = unsafe { clang_getCursorLocation(cursor) };

    if unsafe { clang_Location_isFromMainFile(location) } == 0 {
        return CXChildVisit_Recurse;
    }
    
    let kind = unsafe { clang_getCursorKind(cursor) };

    let original_function_qname = context.current_parent_function_qname.clone();
    let original_class_name = context.current_parent_class_name.clone();

    match kind {
        CXCursor_ClassDecl | CXCursor_StructDecl => {
            let name = cxstring_to_string_log(unsafe { clang_getCursorSpelling(cursor) });
            context.current_parent_class_name = Some(name);
        }
        CXCursor_FunctionDecl | CXCursor_CXXMethod => {
            if unsafe { clang_isCursorDefinition(cursor) } != 0 {
                let name = cxstring_to_string_log(unsafe { clang_getCursorSpelling(cursor) });
                let usr = cxstring_to_string_log(unsafe { clang_getCursorUSR(cursor) });
                let qualified_name = if usr.is_empty() { name.clone() } else { usr };
                context.current_parent_function_qname = Some(qualified_name);
            }
        }
        CXCursor_CallExpr => {
            if let Some(ref parent_func_qname) = context.current_parent_function_qname {
                let referenced_decl_cursor = unsafe { clang_getCursorReferenced(cursor) };
                let callee_name = cxstring_to_string_log(unsafe { clang_getCursorSpelling(referenced_decl_cursor) });

                let mut log_type = LogType::Unknown;
                let mut log_level = LogLevel::Unknown;
                let mut matched = false;

                if !matched && context.config.log_functions.qt.enabled {
                    if context.config.log_functions.qt.functions.contains(&callee_name) {
                        log_type = LogType::Qt;
                        if callee_name.starts_with("qDebug") { log_level = LogLevel::Debug; }
                        else if callee_name.starts_with("qInfo") { log_level = LogLevel::Info; }
                        else if callee_name.starts_with("qWarning") { log_level = LogLevel::Warning; }
                        else if callee_name.starts_with("qCritical") { log_level = LogLevel::Critical; }
                        else if callee_name.starts_with("qFatal") { log_level = LogLevel::Fatal; }
                        matched = true;
                    }
                }

                if !matched && context.config.log_functions.qt.enabled {
                     if context.config.log_functions.qt.category_functions.contains(&callee_name) {
                        log_type = LogType::QtCategory;
                        if callee_name.starts_with("qCDebug") { log_level = LogLevel::Debug; }
                        else if callee_name.starts_with("qCInfo") { log_level = LogLevel::Info; }
                        else if callee_name.starts_with("qCWarning") { log_level = LogLevel::Warning; }
                        else if callee_name.starts_with("qCCritical") { log_level = LogLevel::Critical; }
                        matched = true;
                    }
                }
                
                if !matched && context.config.log_functions.custom.enabled {
                    for (level_str, func_names) in &context.config.log_functions.custom.functions {
                        if func_names.contains(&callee_name) {
                            log_type = LogType::Custom;
                            log_level = match level_str.to_lowercase().as_str() {
                                "debug" => LogLevel::Debug,
                                "info" => LogLevel::Info,
                                "warning" | "warn" => LogLevel::Warning,
                                "error" | "critical" => LogLevel::Critical,
                                "fatal" => LogLevel::Fatal,
                                _ => LogLevel::Unknown,
                            };
                            matched = true;
                            break; 
                        }
                    }
                }

                if matched {
                    let call_loc = get_source_location_log(unsafe { clang_getCursorLocation(cursor) });
                    
                    let mut message_arguments = Vec::new();
                    let num_args = unsafe { clang_Cursor_getNumArguments(cursor) };
                    for i in 0..num_args {
                        let arg_cursor = unsafe { clang_Cursor_getArgument(cursor, i as u32) };
                        let arg_text = cxstring_to_string_log(unsafe { clang_getCursorSpelling(arg_cursor) });
                        message_arguments.push(if !arg_text.is_empty() { arg_text } else { "<complex_arg>".to_string() });
                    }

                    let log_call = LogCallSite {
                        function_name: callee_name,
                        source_location: call_loc,
                        log_level,
                        log_type,
                        parent_function_qualified_name: parent_func_qname.clone(),
                        containing_class_name: context.current_parent_class_name.clone(),
                        message_arguments,
                    };
                    debug!("Identified log call: {:?}", log_call);
                    context.log_call_sites.push(log_call);
                }
            }
        }
        _ => {}
    }

    unsafe {
        clang_visitChildren(
            cursor,
            visit_log_identifier_cursor,
            client_data,
        );
    }

    context.current_parent_function_qname = original_function_qname;
    context.current_parent_class_name = original_class_name;

    CXChildVisit_Continue
}
