mod utils;
mod config;
mod cli;
mod source_manager;
mod core; 
mod reporter; // Added reporter module

use config::ConfigManager;
use cli::parse_arguments;
use source_manager::SourceManager;
use core::ast_analyzer::AstAnalyzer; 
use core::log_identifier::LogIdentifier;
use core::coverage::CoverageCalculator;
use reporter::get_reporter; // Use the reporter factory
use utils::log_utils;
use utils::file_utils as app_file_utils; 
use std::collections::HashMap;
use std::path::PathBuf;
use std::fs::File as StdFile; // Alias to avoid conflict with crate::File if any
use std::io::{BufWriter, Write}; // For writing reports
use log::{info, error, debug, warn};

fn main() {
    log_utils::init_logger(); 

    info!("Parsing command line arguments...");
    let cli_options = parse_arguments();
    info!("Parsed CLI options: {:?}", cli_options);

    if let Some(log_level_str) = &cli_options.log_level {
        warn!("CLI log level ('{}') specified. For env_logger, this typically requires RUST_LOG to be set *before* application start. Current init_logger initializes on first call.", log_level_str);
    }

    info!("Initializing DLogCover-rs application with parsed arguments.");

    match ConfigManager::new(&cli_options) {
        Ok(config_manager) => {
            info!("ConfigManager initialized successfully.");
            let app_config = &config_manager.config; // Get a reference to the config
            debug!("Final effective config: {:?}", app_config);

            info!("Initializing SourceManager...");
            match SourceManager::new(app_config) {
                Ok(mut source_manager) => {
                    info!("SourceManager initialized successfully.");
                    
                    info!("Collecting source files...");
                    match source_manager.collect_source_files() {
                        Ok(()) => {
                            info!("Successfully collected source files.");
                            let collected_files = source_manager.get_source_files();
                            info!("Number of source files collected: {}", collected_files.len());
                            
                            if collected_files.is_empty() {
                                info!("No source files to analyze.");
                            } else {
                                // Simplified logging of collected files for brevity
                                if collected_files.len() <= 5 {
                                     debug!("Collected files: {:?}", collected_files.iter().map(|sf| sf.absolute_path.display()).collect::<Vec<_>>());
                                } else {
                                     debug!("Collected files (first 5): {:?}", collected_files.iter().take(5).map(|sf| sf.absolute_path.display()).collect::<Vec<_>>());
                                }


                                // --- AST Analysis ---
                                info!("Initializing AstAnalyzer...");
                                let mut ast_analyzer = AstAnalyzer::new(app_config);
                                info!("AstAnalyzer initialized. Starting analysis...");
                                match ast_analyzer.analyze_files(collected_files) {
                                    Ok(()) => {
                                        info!("AST analysis completed successfully.");
                                        let ast_results = ast_analyzer.get_analysis_results();
                                        info!("Number of files with AST info: {}", ast_results.len());
                                        
                                        // --- Log Identification ---
                                        info!("Initializing LogIdentifier...");
                                        let log_identifier = LogIdentifier::new(app_config);
                                        info!("LogIdentifier initialized. Identifying log calls...");

                                        let mut log_sites_map = HashMap::new();

                                        for (file_path, ast_info_ref) in ast_results { 
                                            debug!("Processing file for log identification: {}", file_path.display());
                                            match app_file_utils::read_file(file_path) {
                                                Ok(file_content) => {
                                                    match log_identifier.identify_log_calls_in_file(ast_info_ref, &file_content) {
                                                        Ok(log_calls) => {
                                                            info!("Found {} log calls in {}", log_calls.len(), file_path.display());
                                                            log_sites_map.insert(file_path.clone(), log_calls);
                                                        }
                                                        Err(e) => {
                                                            error!("Failed to identify log calls in {}: {}", file_path.display(), e);
                                                        }
                                                    }
                                                }
                                                Err(e) => {
                                                    error!("Failed to read file content for {}: {}", file_path.display(), e);
                                                }
                                            }
                                        }

                                        // --- Coverage Calculation ---
                                        if !ast_results.is_empty() {
                                            info!("Initializing CoverageCalculator...");
                                            let coverage_calculator = CoverageCalculator::new(app_config);
                                            info!("CoverageCalculator initialized. Calculating project coverage...");

                                            match coverage_calculator.calculate_project_coverage(ast_results, &log_sites_map) {
                                                Ok(project_coverage) => {
                                                    info!("Project coverage calculated successfully.");
                                                    debug!("Project Coverage Details: {:?}", project_coverage);
                                                    
                                                    // --- Report Generation ---
                                                    info!("Generating report (format: {})...", app_config.report.format);
                                                    if let Some(reporter) = get_reporter(&app_config.report.format) {
                                                        let mut writer: Box<dyn Write> = match &cli_options.output {
                                                            Some(output_path_str) => {
                                                                let output_path = PathBuf::from(output_path_str);
                                                                info!("Report will be written to: {}", output_path.display());
                                                                // Ensure parent directory exists
                                                                if let Some(parent_dir) = output_path.parent() {
                                                                    if !parent_dir.exists() {
                                                                        if let Err(e) = std::fs::create_dir_all(parent_dir) {
                                                                            error!("Failed to create parent directory for report '{}': {}", parent_dir.display(), e);
                                                                            // Fallback to stdout or error out? For now, error out.
                                                                            // Consider making this behavior configurable or more robust.
                                                                            Box::new(std::io::sink()) // Fallback to sink on error
                                                                        } else {
                                                                             match StdFile::create(output_path) {
                                                                                Ok(file) => Box::new(BufWriter::new(file)),
                                                                                Err(e) => {
                                                                                    error!("Failed to create report file '{}': {}. Falling back to stdout.", output_path_str, e);
                                                                                    Box::new(BufWriter::new(std::io::stdout()))
                                                                                }
                                                                            }
                                                                        }
                                                                    } else {
                                                                         match StdFile::create(output_path) {
                                                                            Ok(file) => Box::new(BufWriter::new(file)),
                                                                            Err(e) => {
                                                                                error!("Failed to create report file '{}': {}. Falling back to stdout.", output_path_str, e);
                                                                                Box::new(BufWriter::new(std::io::stdout()))
                                                                            }
                                                                        }
                                                                    }
                                                                } else { // No parent dir (e.g. "report.txt" in current dir)
                                                                    match StdFile::create(output_path) {
                                                                        Ok(file) => Box::new(BufWriter::new(file)),
                                                                        Err(e) => {
                                                                            error!("Failed to create report file '{}': {}. Falling back to stdout.", output_path_str, e);
                                                                            Box::new(BufWriter::new(std::io::stdout()))
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                            None => {
                                                                info!("Report will be written to stdout.");
                                                                Box::new(BufWriter::new(std::io::stdout()))
                                                            }
                                                        };

                                                        if let Err(e) = reporter.generate_report(&project_coverage, &mut writer) {
                                                            error!("Failed to generate report: {}", e);
                                                        } else {
                                                            info!("Report generated successfully.");
                                                            // Ensure buffer is flushed if writer is BufWriter
                                                            if let Err(e) = writer.flush() {
                                                                error!("Failed to flush report writer: {}", e);
                                                            }
                                                        }
                                                    } else {
                                                        error!("Unsupported report format: {}", app_config.report.format);
                                                    }

                                                }
                                                Err(e) => {
                                                    error!("Failed to calculate project coverage: {}", e);
                                                }
                                            }
                                        } else {
                                            info!("Skipping coverage calculation and reporting as no AST results were available.");
                                        }
                                    }
                                    Err(e) => {
                                        error!("AST analysis failed: {}", e);
                                    }
                                }
                            }
                        }
                        Err(e) => {
                            error!("Failed to collect source files: {}", e);
                        }
                    }
                }
                Err(e) => {
                    error!("Failed to initialize SourceManager: {}", e);
                }
            }
        }
        Err(e) => {
            error!("Failed to initialize ConfigManager: {}", e);
        }
    }

    info!("DLogCover-rs processing finished."); 
}
