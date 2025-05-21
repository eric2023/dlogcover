use log::LevelFilter; // Removed Level
use env_logger::{Builder, Env};
use std::sync::Once;

#[allow(dead_code)]
static INIT: Once = Once::new();

#[allow(dead_code)]
pub fn init_logger() {
    INIT.call_once(|| {
        Builder::from_env(Env::default().default_filter_or("info"))
            .try_init()
            .expect("Failed to initialize logger");
        log::info!("Logger initialized via env_logger.");
    });
}

#[allow(dead_code)]
pub fn set_log_level(_level: LevelFilter) { // Prefixed level with _
    log::warn!("Dynamic log level setting with env_logger is typically handled by RUST_LOG at startup. Current level: {:?}", log::max_level());
    // log::set_max_level(_level); // This affects the global max level for the `log` facade.
}

#[allow(dead_code)]
pub fn get_log_level() -> LevelFilter {
    log::max_level()
}

#[allow(dead_code)]
pub fn shutdown_logger() {
    log::info!("Logger shutdown requested (env_logger typically auto-flushes).");
}

