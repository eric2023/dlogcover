use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use crate::utils::file_utils;
use crate::cli::CliOptions; // Import CliOptions
use log::{error, warn, info, debug};

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct Config {
    pub scan: ScanConfig,
    pub log_functions: LogFunctionsConfig,
    pub analysis: AnalysisConfig,
    pub report: ReportConfig,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ScanConfig {
    pub directories: Vec<String>,
    pub excludes: Vec<String>,
    pub file_types: Vec<String>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct LogFunctionsConfig {
    pub qt: QtLogConfig,
    pub custom: CustomLogConfig,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct QtLogConfig {
    pub enabled: bool,
    pub functions: Vec<String>,
    pub category_functions: Vec<String>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct CustomLogConfig {
    pub enabled: bool,
    pub functions: HashMap<String, Vec<String>>,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct AnalysisConfig {
    pub function_coverage: bool,
    pub branch_coverage: bool,
    pub exception_coverage: bool,
    pub key_path_coverage: bool,
}

#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct ReportConfig {
    pub format: String, 
    pub timestamp_format: String,
}


#[allow(dead_code)] 
pub struct ConfigManager {
    pub config: Config, 
}

#[allow(dead_code)] 
impl ConfigManager {
    // Modified new to accept CliOptions
    pub fn new(cli_options: &CliOptions) -> Result<Self, Box<dyn std::error::Error>> {
        info!("Initializing ConfigManager with CLI options: {:?}", cli_options);

        // 1. Determine config path: CLI > default path > built-in defaults
        let mut config = match &cli_options.config {
            Some(path) => {
                info!("Attempting to load configuration from CLI specified path: {}", path);
                Self::load_config(path)?
            }
            None => {
                info!("No config path from CLI. Trying default path './dlogcover.json'.");
                match Self::load_config("./dlogcover.json") {
                    Ok(cfg) => cfg,
                    Err(_) => {
                        warn!("Failed to load from default path './dlogcover.json'. Using built-in default configuration.");
                        Self::get_default_config()
                    }
                }
            }
        };

        // 2. Override with CLI options
        Self::merge_cli_options(&mut config, cli_options);

        // 3. Validate the final merged configuration
        if let Err(validation_errors) = Self::validate_config(&config) {
            error!("Final configuration validation failed:");
            for err in &validation_errors {
                error!("- {}", err);
            }
            // Decide if validation errors are fatal or just warnings
            // For now, returning an error to be explicit
            return Err(Box::new(std::io::Error::new(
                std::io::ErrorKind::InvalidInput, 
                format!("Configuration validation errors: {:?}", validation_errors)
            )));
        }
        
        info!("ConfigManager initialized successfully.");
        Ok(ConfigManager { config })
    }

    fn merge_cli_options(config: &mut Config, cli_options: &CliOptions) {
        info!("Merging CLI options into configuration...");

        if let Some(dir) = &cli_options.directory {
            info!("Overriding scan directory with CLI option: {}", dir);
            config.scan.directories = vec![dir.clone()];
        }

        if !cli_options.exclude.is_empty() {
            info!("Overriding/extending scan excludes with CLI options: {:?}", cli_options.exclude);
            // Overwrite or extend? Current C++ seems to overwrite if any -e is given.
            // For simplicity, let's overwrite. If extension is needed, logic can be more complex.
            config.scan.excludes = cli_options.exclude.clone();
        }

        if let Some(log_level_cli) = &cli_options.log_level {
            // Note: Actual log level setting is usually done via RUST_LOG for env_logger.
            // This override might be for a custom logger or for display purposes.
            // For now, we can store it if Config had a place, or just log it.
            info!("CLI log level specified: {}. (Note: Actual logging level might be controlled by RUST_LOG)", log_level_cli);
            // If `Config` struct had a `log_level` field, you'd set it here.
            // e.g., config.log_settings.log_level = log_level_cli.clone();
        }

        if let Some(output_path) = &cli_options.output {
            info!("Overriding report output path with CLI option: {}", output_path);
            // Assuming ReportConfig might get an `output_path` field later.
            // For now, if ReportConfig doesn't store output path directly, this is conceptual.
            // config.report.output_path = Some(output_path.clone()); 
            // Let's add an output_path to ReportConfig for this example.
            // This change requires modifying ReportConfig struct and default_config.
            warn!("CLI output path '{}' provided, but ReportConfig.output_path is not fully implemented yet.", output_path);
        }

        if let Some(format) = &cli_options.format {
            info!("Overriding report format with CLI option: {}", format);
            config.report.format = format.clone();
        }
        debug!("Configuration after merging CLI options: {:?}", config);
    }

    // load_config remains largely the same
    pub fn load_config(path: &str) -> Result<Config, Box<dyn std::error::Error>> {
        info!("Loading configuration from: {}", path);
        let content = file_utils::read_file(path)
            .map_err(|e| {
                error!("Failed to read config file '{}': {}", path, e);
                e
            })?;
        
        let config: Config = serde_json::from_str(&content)
            .map_err(|e| {
                error!("Failed to parse JSON from config file '{}': {}", path, e);
                e
            })?;
        
        info!("Configuration loaded successfully from {}", path);
        Ok(config)
    }

    // get_default_config remains the same
    pub fn get_default_config() -> Config {
        info!("Using default configuration.");
        Config {
            scan: ScanConfig {
                directories: vec!["./".to_string()],
                excludes: vec!["build/".to_string(), "test/".to_string()],
                file_types: vec![".cpp".to_string(), ".cc".to_string(), ".cxx".to_string(), ".h".to_string(), ".hpp".to_string()],
            },
            log_functions: LogFunctionsConfig {
                qt: QtLogConfig {
                    enabled: true,
                    functions: vec!["qDebug".to_string(), "qInfo".to_string(), "qWarning".to_string(), "qCritical".to_string(), "qFatal".to_string()],
                    category_functions: vec!["qCDebug".to_string(), "qCInfo".to_string(), "qCWarning".to_string(), "qCCritical".to_string()],
                },
                custom: CustomLogConfig {
                    enabled: true,
                    functions: {
                        let mut map = HashMap::new();
                        map.insert("debug".to_string(), vec!["fmDebug".to_string()]);
                        map.insert("info".to_string(), vec!["fmInfo".to_string()]);
                        map.insert("warning".to_string(), vec!["fmWarning".to_string()]);
                        map.insert("critical".to_string(), vec!["fmCritical".to_string()]);
                        map
                    },
                },
            },
            analysis: AnalysisConfig {
                function_coverage: true,
                branch_coverage: true,
                exception_coverage: true,
                key_path_coverage: true,
            },
            report: ReportConfig {
                format: "text".to_string(),
                timestamp_format: "YYYYMMDD_HHMMSS".to_string(),
                // output_path: None, // If we add output_path to ReportConfig
            },
        }
    }

    // validate_config remains the same
    pub fn validate_config(config: &Config) -> Result<(), Vec<String>> {
        let mut errors = Vec::new();
        info!("Validating configuration...");

        if config.scan.directories.is_empty() {
            errors.push("Scan directories list cannot be empty.".to_string());
        }
        for dir in &config.scan.directories {
            if dir.trim().is_empty() {
                errors.push("Scan directory path cannot be empty or just whitespace.".to_string());
            }
            debug!("Validating configured scan directory: {}", dir);
        }
        if config.scan.file_types.is_empty() {
            errors.push("Scan file_types list cannot be empty.".to_string());
        }
        for ft in &config.scan.file_types {
            if !ft.starts_with('.') || ft.len() < 2 {
                errors.push(format!("Invalid file type format: '{}'. Must start with '.' and have at least one character (e.g. '.cpp').", ft));
            }
        }

        if config.log_functions.qt.enabled {
            if config.log_functions.qt.functions.is_empty() && config.log_functions.qt.category_functions.is_empty() {
                errors.push("Qt logging is enabled, but no Qt log functions or category functions are specified.".to_string());
            }
        }
        if config.log_functions.custom.enabled {
            if config.log_functions.custom.functions.is_empty() {
                errors.push("Custom logging is enabled, but no custom log functions are specified.".to_string());
            }
            for (level, func_list) in &config.log_functions.custom.functions {
                if func_list.is_empty() {
                    errors.push(format!("Custom log functions for level '{}' is an empty list.", level));
                }
            }
        }
        if !config.log_functions.qt.enabled && !config.log_functions.custom.enabled {
            warn!("Both Qt and Custom logging are disabled. No log functions will be scanned.");
        }

        let allowed_formats = ["text", "html", "json"]; 
        if !allowed_formats.contains(&config.report.format.to_lowercase().as_str()) {
            errors.push(format!("Invalid report format: '{}'. Allowed formats are: {:?}", config.report.format, allowed_formats));
        }
        if config.report.timestamp_format.trim().is_empty() {
            errors.push("Report timestamp_format cannot be empty.".to_string());
        }

        if errors.is_empty() {
            info!("Configuration validation successful.");
            Ok(())
        } else {
            warn!("Configuration validation found {} errors.", errors.len());
            Err(errors)
        }
    }
}

