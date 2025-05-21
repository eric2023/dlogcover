use crate::core::coverage::ProjectCoverage;
use crate::core::ast_analyzer::SourceLocation; // Used for formatting locations
use std::io::Write;
use std::io::Error as IoError; // Alias for clarity

pub trait ReporterStrategy {
    fn generate_report(
        &self,
        project_coverage: &ProjectCoverage,
        writer: &mut dyn Write,
    ) -> Result<(), IoError>;

    // Optional helper for formatting SourceLocation, can be a default method
    // or implemented by each concrete reporter if formatting differs.
    // For now, let's assume each reporter handles its own location formatting if needed.
    // fn format_location(&self, location: &SourceLocation) -> String {
    //     format!("{}:L{}C{}", location.file_path.display(), location.line, location.column)
    // }
}
