pub mod coverage_calculator;

// Re-export key structs and the calculator itself for easier access
pub use coverage_calculator::{
    CoverageCalculator, 
    ProjectCoverage, 
    PerFileCoverage, 
    CoverageMetrics
};
