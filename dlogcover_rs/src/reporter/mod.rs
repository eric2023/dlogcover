pub mod reporter_strategy;
pub mod text_reporter;
pub mod json_reporter;

pub use reporter_strategy::ReporterStrategy;
use text_reporter::TextReporter;
use json_reporter::JsonReporter;

#[allow(dead_code)] // Will be used by main.rs
pub fn get_reporter(format: &str) -> Option<Box<dyn ReporterStrategy>> {
    match format.to_lowercase().as_str() {
        "text" => Some(Box::new(TextReporter)),
        "json" => Some(Box::new(JsonReporter)),
        _ => None,
    }
}
