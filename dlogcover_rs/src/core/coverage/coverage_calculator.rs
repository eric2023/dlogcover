use std::path::PathBuf;
use std::collections::HashMap;
use crate::core::ast_analyzer::{SourceLocation, FunctionInfo, BranchInfo, ExceptionInfo, FileAstInfo};
use crate::core::log_identifier::LogCallSite;
use crate::config::Config;
use log::{debug, info, warn};
use serde::Serialize; // Added Serialize

// --- Coverage Statistics Structs ---

#[derive(Debug, Clone, Default, Serialize)] // Added Serialize
#[allow(dead_code)]
pub struct CoverageMetrics {
    pub total: usize,
    pub covered: usize,
    pub percentage: f64,
}

impl CoverageMetrics {
    #[allow(dead_code)]
    pub fn new(total: usize, covered: usize) -> Self {
        let percentage = if total > 0 {
            (covered as f64 / total as f64) * 100.0
        } else {
            100.0 
        };
        CoverageMetrics { total, covered, percentage }
    }

    #[allow(dead_code)]
    pub fn calculate_percentage(&mut self) {
        self.percentage = if self.total > 0 {
            (self.covered as f64 / self.total as f64) * 100.0
        } else {
            100.0 
        };
    }
}

#[derive(Debug, Clone, Serialize)] // Added Serialize
#[allow(dead_code)]
pub struct PerFileCoverage {
    pub file_path: PathBuf,
    pub functions: CoverageMetrics,
    pub branches: CoverageMetrics,
    pub exceptions: CoverageMetrics,
    pub overall: CoverageMetrics,
    pub uncovered_functions: Vec<FunctionInfo>, 
    pub uncovered_branches: Vec<BranchInfo>,
    pub uncovered_exceptions: Vec<ExceptionInfo>,
}

impl PerFileCoverage {
    #[allow(dead_code)]
    pub fn new(file_path: PathBuf) -> Self {
        PerFileCoverage {
            file_path,
            functions: CoverageMetrics::default(),
            branches: CoverageMetrics::default(),
            exceptions: CoverageMetrics::default(),
            overall: CoverageMetrics::default(),
            uncovered_functions: Vec::new(),
            uncovered_branches: Vec::new(),
            uncovered_exceptions: Vec::new(),
        }
    }
}

#[derive(Debug, Clone, Default, Serialize)] // Added Serialize
#[allow(dead_code)]
pub struct ProjectCoverage {
    pub files: Vec<PerFileCoverage>,
    pub total_functions: CoverageMetrics,
    pub total_branches: CoverageMetrics,
    pub total_exceptions: CoverageMetrics,
    pub project_overall: CoverageMetrics,
}

// --- CoverageCalculator Struct ---
#[allow(dead_code)]
pub struct CoverageCalculator<'a> {
    config: &'a Config,
}

#[allow(dead_code)]
impl<'a> CoverageCalculator<'a> {
    pub fn new(config: &'a Config) -> Self {
        debug!("Initializing CoverageCalculator...");
        CoverageCalculator { config }
    }

    fn is_location_within_range(
        log_loc: &SourceLocation,
        item_start_loc: &SourceLocation,
        item_end_loc: &SourceLocation,
    ) -> bool {
        if log_loc.line < item_start_loc.line || log_loc.line > item_end_loc.line {
            return false;
        }
        if log_loc.line == item_start_loc.line && log_loc.column < item_start_loc.column {
            return false;
        }
        if log_loc.line == item_end_loc.line && log_loc.column > item_end_loc.column {
            return false;
        }
        true
    }

    fn calculate_file_coverage(
        &self,
        file_ast: &FileAstInfo,
        log_sites: &[LogCallSite],
    ) -> PerFileCoverage {
        info!("Calculating coverage for file: {}", file_ast.file_path.display());
        let mut file_coverage = PerFileCoverage::new(file_ast.file_path.clone());

        file_coverage.functions.total = file_ast.functions.iter().filter(|f| f.has_body).count();
        for func_info in file_ast.functions.iter().filter(|f| f.has_body) {
            let is_covered = log_sites.iter().any(|log_site| {
                log_site.parent_function_qualified_name == func_info.qualified_name ||
                Self::is_location_within_range(&log_site.source_location, &func_info.start_location, &func_info.end_location)
            });

            if is_covered {
                file_coverage.functions.covered += 1;
            } else {
                file_coverage.uncovered_functions.push(func_info.clone());
            }
        }
        file_coverage.functions.calculate_percentage();
        debug!("  Function coverage: {}/{} ({:.2}%)", file_coverage.functions.covered, file_coverage.functions.total, file_coverage.functions.percentage);

        file_coverage.branches.total = file_ast.branches.len();
        for branch_info in &file_ast.branches {
            let is_covered = log_sites.iter().any(|log_site| {
                Self::is_location_within_range(&log_site.source_location, &branch_info.start_location, &branch_info.end_location)
            });
            if is_covered {
                file_coverage.branches.covered += 1;
            } else {
                file_coverage.uncovered_branches.push(branch_info.clone());
            }
        }
        file_coverage.branches.calculate_percentage();
        debug!("  Branch coverage: {}/{} ({:.2}%)", file_coverage.branches.covered, file_coverage.branches.total, file_coverage.branches.percentage);

        file_coverage.exceptions.total = file_ast.exceptions.len();
        for exc_info in &file_ast.exceptions {
            let is_covered = log_sites.iter().any(|log_site| {
                Self::is_location_within_range(&log_site.source_location, &exc_info.start_location, &exc_info.end_location)
            });
            if is_covered {
                file_coverage.exceptions.covered += 1;
            } else {
                file_coverage.uncovered_exceptions.push(exc_info.clone());
            }
        }
        file_coverage.exceptions.calculate_percentage();
        debug!("  Exception coverage: {}/{} ({:.2}%)", file_coverage.exceptions.covered, file_coverage.exceptions.total, file_coverage.exceptions.percentage);

        let total_items = file_coverage.functions.total + file_coverage.branches.total + file_coverage.exceptions.total;
        let covered_items = file_coverage.functions.covered + file_coverage.branches.covered + file_coverage.exceptions.covered;
        file_coverage.overall = CoverageMetrics::new(total_items, covered_items);
        debug!("  Overall file coverage: {}/{} ({:.2}%)", file_coverage.overall.covered, file_coverage.overall.total, file_coverage.overall.percentage);

        file_coverage
    }

    pub fn calculate_project_coverage(
        &self,
        ast_results: &HashMap<PathBuf, FileAstInfo>,
        log_sites_map: &HashMap<PathBuf, Vec<LogCallSite>>,
    ) -> Result<ProjectCoverage, String> {
        info!("Calculating project-wide coverage...");
        let mut project_coverage = ProjectCoverage::default();

        if ast_results.is_empty() {
            warn!("No AST results provided for project coverage calculation. Returning empty coverage.");
            return Ok(project_coverage);
        }

        for (file_path, file_ast_info) in ast_results {
            let empty_log_sites = Vec::new(); 
            let log_sites_for_file = log_sites_map.get(file_path).unwrap_or(&empty_log_sites);
            
            debug!("Processing file for project coverage: {}", file_path.display());
            let file_cov = self.calculate_file_coverage(file_ast_info, log_sites_for_file);

            project_coverage.total_functions.total += file_cov.functions.total;
            project_coverage.total_functions.covered += file_cov.functions.covered;
            project_coverage.total_branches.total += file_cov.branches.total;
            project_coverage.total_branches.covered += file_cov.branches.covered;
            project_coverage.total_exceptions.total += file_cov.exceptions.total;
            project_coverage.total_exceptions.covered += file_cov.exceptions.covered;
            
            project_coverage.files.push(file_cov);
        }

        project_coverage.total_functions.calculate_percentage();
        project_coverage.total_branches.calculate_percentage();
        project_coverage.total_exceptions.calculate_percentage();

        let overall_total_items = project_coverage.total_functions.total + 
                                  project_coverage.total_branches.total + 
                                  project_coverage.total_exceptions.total;
        let overall_covered_items = project_coverage.total_functions.covered + 
                                    project_coverage.total_branches.covered + 
                                    project_coverage.total_exceptions.covered;
        
        project_coverage.project_overall = CoverageMetrics::new(overall_total_items, overall_covered_items);

        info!("Project coverage calculation finished.");
        debug!("Project Functions: {}/{} ({:.2}%)", project_coverage.total_functions.covered, project_coverage.total_functions.total, project_coverage.total_functions.percentage);
        debug!("Project Branches: {}/{} ({:.2}%)", project_coverage.total_branches.covered, project_coverage.total_branches.total, project_coverage.total_branches.percentage);
        debug!("Project Exceptions: {}/{} ({:.2}%)", project_coverage.total_exceptions.covered, project_coverage.total_exceptions.total, project_coverage.total_exceptions.percentage);
        debug!("Project Overall: {}/{} ({:.2}%)", project_coverage.project_overall.covered, project_coverage.project_overall.total, project_coverage.project_overall.percentage);
        
        Ok(project_coverage)
    }
}
