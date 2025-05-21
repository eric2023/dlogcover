use clap::Parser;

#[allow(dead_code)]
#[derive(Parser, Debug)]
#[command(name = "dlogcover-rs", version = "0.1.0", about = "DLogCover-rs: A tool for DLT log coverage analysis.", long_about = None)]
pub struct CliOptions {
    /// Specifies the directory to scan for log files or source code.
    #[arg(short, long, value_name = "PATH")]
    pub directory: Option<String>,

    /// Specifies the output path for reports or processed files.
    #[arg(short, long, value_name = "PATH")]
    pub output: Option<String>,

    /// Specifies the path to a custom configuration file (e.g., dlogcover.json).
    #[arg(short, long, value_name = "FILE_PATH")]
    pub config: Option<String>,

    /// Excludes files or directories matching the specified pattern. Can be used multiple times.
    #[arg(short, long, value_name = "PATTERN", num_args = 0..)]
    pub exclude: Vec<String>,

    /// Sets the logging level (e.g., error, warn, info, debug, trace).
    #[arg(short = 'L', long = "log-level", value_name = "LEVEL")]
    pub log_level: Option<String>,

    /// Sets the output format for reports (e.g., text, json, html).
    #[arg(short, long, value_name = "FORMAT")]
    pub format: Option<String>,

    // The version is handled by #[command(version = "...")] or inferred from Cargo.toml
    // No explicit field is needed if we just want clap to print version and exit.
    // If we needed to *programmatically access* whether --version was passed (uncommon),
    // then a field with a specific action might be used, but can be tricky.
}

#[allow(dead_code)]
pub fn parse_arguments() -> CliOptions {
    CliOptions::parse()
}

