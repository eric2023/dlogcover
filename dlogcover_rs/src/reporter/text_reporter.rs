use super::reporter_strategy::ReporterStrategy;
use crate::core::coverage::{ProjectCoverage, PerFileCoverage, CoverageMetrics};
use crate::core::ast_analyzer::{SourceLocation, FunctionInfo, BranchInfo, ExceptionInfo};
use std::io::{Write, Error as IoError, ErrorKind as IoErrorKind};

#[allow(dead_code)] // Will be used by the factory
pub struct TextReporter;

impl TextReporter {
    fn format_source_location(loc: &SourceLocation) -> String {
        // Assuming SourceLocation.file_path is the full absolute path.
        // For text reports, often just the line and column are needed if file context is already given.
        // If file_path needs to be displayed per location, adjust accordingly.
        // For this version, we'll rely on the file context being printed once per PerFileCoverage.
        format!("L{}:C{}", loc.line, loc.column)
    }

    fn format_coverage_metrics(metrics: &CoverageMetrics, item_name: &str) -> String {
        format!(
            "{}: {}/{} ({:.2}%)",
            item_name, metrics.covered, metrics.total, metrics.percentage
        )
    }
}

impl ReporterStrategy for TextReporter {
    fn generate_report(
        &self,
        project_coverage: &ProjectCoverage,
        writer: &mut dyn Write,
    ) -> Result<(), IoError> {
        writeln!(writer, "--- DLogCover-rs Text Report ---")?;
        writeln!(writer, "")?;

        // Project Overall Summary
        writeln!(writer, "Project Coverage Summary:")?;
        writeln!(writer, "  {}", Self::format_coverage_metrics(&project_coverage.total_functions, "Functions"))?;
        writeln!(writer, "  {}", Self::format_coverage_metrics(&project_coverage.total_branches, "Branches"))?;
        writeln!(writer, "  {}", Self::format_coverage_metrics(&project_coverage.total_exceptions, "Exceptions"))?;
        writeln!(writer, "  {}", Self::format_coverage_metrics(&project_coverage.project_overall, "Overall"))?;
        writeln!(writer, "")?;

        if project_coverage.files.is_empty() {
            writeln!(writer, "No files were analyzed.")?;
            return Ok(());
        }

        writeln!(writer, "Per-File Coverage Details:")?;
        for (idx, file_coverage) in project_coverage.files.iter().enumerate() {
            writeln!(writer, "--------------------------------------------------")?;
            writeln!(writer, "[{}/{}] File: {}", idx + 1, project_coverage.files.len(), file_coverage.file_path.display())?;
            writeln!(writer, "  Metrics:")?;
            writeln!(writer, "    {}", Self::format_coverage_metrics(&file_coverage.functions, "Functions"))?;
            writeln!(writer, "    {}", Self::format_coverage_metrics(&file_coverage.branches, "Branches"))?;
            writeln!(writer, "    {}", Self::format_coverage_metrics(&file_coverage.exceptions, "Exceptions"))?;
            writeln!(writer, "    {}", Self::format_coverage_metrics(&file_coverage.overall, "Overall File"))?;
            
            if !file_coverage.uncovered_functions.is_empty() {
                writeln!(writer, "  Uncovered Functions ({}):", file_coverage.uncovered_functions.len())?;
                for func_info in &file_coverage.uncovered_functions {
                    writeln!(
                        writer,
                        "    - Name: {} (QName: {})",
                        func_info.name, func_info.qualified_name
                    )?;
                    writeln!(
                        writer,
                        "      Location: {} - {}",
                        Self::format_source_location(&func_info.start_location),
                        Self::format_source_location(&func_info.end_location)
                    )?;
                    // Minimal details, could add parameters, return type if needed
                }
            } else if file_coverage.functions.total > 0 {
                 writeln!(writer, "  All functions covered.")?;
            }


            if !file_coverage.uncovered_branches.is_empty() {
                writeln!(writer, "  Uncovered Branches ({}):", file_coverage.uncovered_branches.len())?;
                for branch_info in &file_coverage.uncovered_branches {
                    writeln!(
                        writer,
                        "    - Type: {}, Location: {} - {}",
                        branch_info.branch_type,
                        Self::format_source_location(&branch_info.start_location),
                        Self::format_source_location(&branch_info.end_location)
                    )?;
                     if let Some(cond) = &branch_info.condition_expression {
                        writeln!(writer, "      Condition: {}", cond)?;
                    }
                }
            } else if file_coverage.branches.total > 0 {
                writeln!(writer, "  All branches covered.")?;
            }

            if !file_coverage.uncovered_exceptions.is_empty() {
                writeln!(writer, "  Uncovered Exception Blocks ({}):", file_coverage.uncovered_exceptions.len())?;
                for exc_info in &file_coverage.uncovered_exceptions {
                    writeln!(
                        writer,
                        "    - Type: {}, Location: {} - {}",
                        exc_info.exception_type,
                        Self::format_source_location(&exc_info.start_location),
                        Self::format_source_location(&exc_info.end_location)
                    )?;
                    if let Some(caught_type) = &exc_info.caught_type {
                         writeln!(writer, "      Caught Type: {}", caught_type)?;
                    }
                }
            } else if file_coverage.exceptions.total > 0 {
                 writeln!(writer, "  All exception blocks covered.")?;
            }
            writeln!(writer, "")?; // Newline after each file section
        }
        writeln!(writer, "--------------------------------------------------")?;
        writeln!(writer, "End of Report")?;

        Ok(())
    }
}
