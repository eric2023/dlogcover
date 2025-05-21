use std::path::PathBuf; // Removed unused `Path`
use crate::config::Config;
use crate::utils::file_utils;
use regex::Regex;
use walkdir::WalkDir;
use log::{debug, error, info, warn};

#[derive(Debug, Clone)]
#[allow(dead_code)]
pub struct SourceFile {
    pub absolute_path: PathBuf,
    pub relative_path: PathBuf,
}

#[allow(dead_code)]
pub struct SourceManager<'a> {
    config: &'a Config,
    source_files: Vec<SourceFile>,
    exclude_patterns: Vec<Regex>,
}

#[allow(dead_code)]
impl<'a> SourceManager<'a> {
    pub fn new(config: &'a Config) -> Result<Self, String> {
        debug!("Initializing SourceManager...");
        let mut exclude_patterns = Vec::new();
        if config.scan.excludes.is_empty() {
            info!("No exclude patterns specified in configuration.");
        } else {
            for pattern_str in &config.scan.excludes {
                match Regex::new(pattern_str) {
                    Ok(re) => {
                        debug!("Successfully compiled exclude regex: {}", pattern_str);
                        exclude_patterns.push(re);
                    }
                    Err(e) => {
                        let err_msg = format!("Failed to compile exclude regex pattern '{}': {}", pattern_str, e);
                        error!("{}", err_msg);
                        return Err(err_msg);
                    }
                }
            }
            info!("Successfully compiled {} exclude patterns.", exclude_patterns.len());
        }

        Ok(SourceManager {
            config,
            source_files: Vec::new(),
            exclude_patterns,
        })
    }

    pub fn collect_source_files(&mut self) -> Result<(), String> {
        info!("Starting to collect source files...");
        self.source_files.clear();

        if self.config.scan.directories.is_empty() {
            warn!("No scan directories specified in configuration.");
            return Ok(());
        }

        for scan_dir_str in &self.config.scan.directories {
            let scan_dir_config_path = PathBuf::from(scan_dir_str); // Path as specified in config
            
            // Resolve the scan directory to an absolute path for consistent processing
            let absolute_scan_dir = match file_utils::to_absolute_path(&scan_dir_config_path) {
                Ok(abs_path) => {
                    if !abs_path.exists() || !abs_path.is_dir() {
                        warn!("Scan directory '{}' (resolved to '{}') does not exist or is not a directory. Skipping.", scan_dir_config_path.display(), abs_path.display());
                        continue;
                    }
                    info!("Scanning directory: {} (Absolute: {})", scan_dir_config_path.display(), abs_path.display());
                    abs_path
                },
                Err(e) => {
                    warn!("Could not get absolute path for scan directory '{}': {}. Skipping.", scan_dir_config_path.display(), e);
                    continue; 
                }
            };


            for entry_result in WalkDir::new(&absolute_scan_dir).into_iter() {
                let entry = match entry_result {
                    Ok(e) => e,
                    Err(e) => {
                        warn!("Error accessing entry in directory '{}': {}. Skipping.", absolute_scan_dir.display(), e);
                        continue;
                    }
                };

                if !entry.file_type().is_file() {
                    continue;
                }

                let absolute_entry_path = entry.path().to_path_buf();
                
                let file_extension_os = absolute_entry_path.extension();
                let file_extension_str = file_extension_os.and_then(|os| os.to_str());
                
                if let Some(ext_str) = file_extension_str {
                    let dot_ext = format!(".{}", ext_str); // Add dot to match config like ".cpp"
                    if !self.config.scan.file_types.iter().any(|ft| ft.eq_ignore_ascii_case(&dot_ext)) {
                        debug!("Skipping file '{}': extension '{}' not in allowed file types.", absolute_entry_path.display(), dot_ext);
                        continue;
                    }
                } else {
                    // No extension or invalid UTF-8 extension
                    if !self.config.scan.file_types.is_empty() { // Only skip if file_types are specified
                        debug!("Skipping file '{}': no valid extension found.", absolute_entry_path.display());
                        continue; 
                    }
                }

                let path_str_for_regex = absolute_entry_path.to_string_lossy();
                let mut excluded = false;
                for re in &self.exclude_patterns {
                    if re.is_match(&path_str_for_regex) {
                        debug!("Skipping file '{}': matches exclude pattern '{}'", absolute_entry_path.display(), re.as_str());
                        excluded = true;
                        break;
                    }
                }
                if excluded {
                    continue;
                }
                
                // Calculate relative path based on the resolved absolute_scan_dir
                let relative_path = match absolute_entry_path.strip_prefix(&absolute_scan_dir) {
                    Ok(rel_path) => rel_path.to_path_buf(),
                    Err(_) => {
                        warn!("Could not strip prefix '{}' from '{}'. Using file name as relative path.", absolute_scan_dir.display(), absolute_entry_path.display());
                        PathBuf::from(entry.file_name())
                    }
                };
                
                debug!("Collected source file: '{}' (relative to '{}')", absolute_entry_path.display(), absolute_scan_dir.display());
                self.source_files.push(SourceFile {
                    absolute_path: absolute_entry_path,
                    relative_path,
                });
            }
        }
        info!("Finished collecting source files. Total found: {}", self.source_files.len());
        Ok(())
    }

    pub fn get_source_files(&self) -> &Vec<SourceFile> {
        &self.source_files
    }
}
