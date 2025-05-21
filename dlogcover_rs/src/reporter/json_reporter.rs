use super::reporter_strategy::ReporterStrategy;
use crate::core::coverage::ProjectCoverage;
use serde_json; // For JSON serialization
use std::io::{Write, Error as IoError, ErrorKind as IoErrorKind};

#[allow(dead_code)] // Will be used by the factory
pub struct JsonReporter;

impl ReporterStrategy for JsonReporter {
    fn generate_report(
        &self,
        project_coverage: &ProjectCoverage,
        writer: &mut dyn Write,
    ) -> Result<(), IoError> {
        // Use serde_json::to_writer_pretty for a human-readable JSON output.
        // Use serde_json::to_writer for compact JSON.
        match serde_json::to_writer_pretty(writer, project_coverage) {
            Ok(_) => Ok(()),
            Err(serde_err) => {
                // Convert serde_json::Error into std::io::Error
                let err_msg = format!("Failed to serialize project coverage to JSON: {}", serde_err);
                Err(IoError::new(IoErrorKind::InvalidData, err_msg))
            }
        }
    }
}
