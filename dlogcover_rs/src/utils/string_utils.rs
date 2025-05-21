use std::collections::HashMap;
use log::{debug, warn}; // Removed error, assuming log crate is used
// use regex::Regex; // Not used in this direct translation but could be for advanced replaceAll

#[allow(dead_code)]
pub fn split(s: &str, delimiter: &str) -> Vec<String> {
    debug!("Splitting string (len: {}) with delimiter '{}'", s.len(), delimiter);
    if delimiter.is_empty() {
        warn!("Split delimiter is empty. Returning list of characters.");
        return s.chars().map(|c| c.to_string()).collect();
    }
    let parts: Vec<String> = s.split(delimiter)
                              .filter(|sp| !sp.is_empty()) 
                              .map(|sp| sp.to_string())
                              .collect();
    debug!("Split result: {} parts", parts.len());
    parts
}

#[allow(dead_code)]
pub fn join(elements: &[String], delimiter: &str) -> String {
    debug!("Joining {} string elements with delimiter '{}'", elements.len(), delimiter);
    elements.join(delimiter)
}

#[allow(dead_code)]
pub fn replace(s: &str, from: &str, to: &str) -> String {
    debug!("Replacing '{}' with '{}' in string (len: {})", from, to, s.len());
    if from.is_empty() {
        warn!("'from' string in replace is empty. Returning original string.");
        return s.to_string(); 
    }
    s.replace(from, to)
}

#[allow(dead_code)]
pub fn starts_with(s: &str, prefix: &str) -> bool {
    let result = s.starts_with(prefix);
    debug!("Checking if string (len: {}) starts with '{}': {}", s.len(), prefix, result);
    result
}

#[allow(dead_code)]
pub fn ends_with(s: &str, suffix: &str) -> bool {
    let result = s.ends_with(suffix);
    debug!("Checking if string (len: {}) ends with '{}': {}", s.len(), suffix, result);
    result
}

#[allow(dead_code)]
pub fn to_lower(s: &str) -> String {
    debug!("Converting string (len: {}) to lowercase", s.len());
    s.to_lowercase()
}

#[allow(dead_code)]
pub fn to_upper(s: &str) -> String {
    debug!("Converting string (len: {}) to uppercase", s.len());
    s.to_uppercase()
}

#[allow(dead_code)]
pub fn trim(s: &str) -> String {
    debug!("Trimming whitespace from string (len: {})", s.len());
    s.trim().to_string()
}

#[allow(dead_code)]
pub fn trim_left(s: &str) -> String {
    debug!("Trimming whitespace from left of string (len: {})", s.len());
    s.trim_start().to_string()
}

#[allow(dead_code)]
pub fn trim_right(s: &str) -> String {
    debug!("Trimming whitespace from right of string (len: {})", s.len());
    s.trim_end().to_string()
}


#[allow(dead_code)]
pub fn try_parse_int(s: &str) -> Option<i32> {
    debug!("Attempting to parse string '{}' as i32", s);
    match s.trim().parse::<i32>() {
        Ok(val) => {
            debug!("Successfully parsed '{}' to i32: {}", s, val);
            Some(val)
        }
        Err(e) => {
            warn!("Failed to parse string '{}' as i32: {}. Original string was: '{}'", s.trim(), e, s);
            None
        }
    }
}

#[allow(dead_code)]
pub fn try_parse_double(s: &str) -> Option<f64> {
    debug!("Attempting to parse string '{}' as f64", s);
    match s.trim().parse::<f64>() {
        Ok(val) => {
            debug!("Successfully parsed '{}' to f64: {}", s, val);
            Some(val)
        }
        Err(e) => {
            warn!("Failed to parse string '{}' as f64: {}. Original string was: '{}'", s.trim(), e, s);
            None
        }
    }
}

#[allow(dead_code)]
pub fn repeat(s: &str, count: usize) -> String {
    debug!("Repeating string (len: {}) {} times", s.len(), count);
    s.repeat(count)
}

#[allow(dead_code)]
pub fn contains_substring(s: &str, sub: &str) -> bool {
    let result = s.contains(sub);
    debug!("Checking if string (len: {}) contains substring '{}': {}", s.len(), sub, result);
    result
}

#[allow(dead_code)]
pub fn replace_all(s: &str, replacements: &HashMap<String, String>) -> String {
    debug!("Performing {} replacements in string (len: {})", replacements.len(), s.len());
    let mut result = s.to_string();
    for (from, to) in replacements {
        if from.is_empty() {
            warn!("'from' string in replace_all is empty for key '{}'. Skipping this replacement.", from);
            continue;
        }
        result = result.replace(from, to);
    }
    result
}

#[allow(dead_code)]
pub fn is_empty(s: &str) -> bool {
    s.is_empty()
}

