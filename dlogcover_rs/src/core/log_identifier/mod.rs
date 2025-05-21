pub mod log_identifier;

// Re-export key structs and the identifier itself for easier access
pub use log_identifier::{
    LogIdentifier, 
    LogCallSite, 
    LogLevel, 
    LogType
};
