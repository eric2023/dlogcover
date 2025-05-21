use std::fs::{self, File};
use std::io::{self}; 
use std::path::{Path, PathBuf};
use std::sync::Mutex; 
use regex::Regex;
use walkdir::WalkDir;
use rand::Rng; 

use log::{debug, error, info, warn};

lazy_static::lazy_static! {
    static ref TEMP_FILES_TO_CLEANUP: Mutex<Vec<PathBuf>> = Mutex::new(Vec::new());
}

#[allow(dead_code)] 
fn log_fs_error<P: AsRef<Path>>(operation: &str, path: P, e: &io::Error) {
    error!("{} at path '{}': {}", operation, path.as_ref().display(), e);
}

#[allow(dead_code)]
pub fn file_exists(path: impl AsRef<Path>) -> bool {
    let path_ref = path.as_ref();
    let exists = path_ref.exists() && path_ref.is_file();
    debug!("Checking if file exists: '{}', result: {}", path_ref.display(), exists);
    exists
}

#[allow(dead_code)]
pub fn directory_exists(path: impl AsRef<Path>) -> bool {
    let path_ref = path.as_ref();
    let exists = path_ref.exists() && path_ref.is_dir();
    debug!("Checking if directory exists: '{}', result: {}", path_ref.display(), exists);
    exists
}

#[allow(dead_code)]
pub fn create_directory(path: impl AsRef<Path>) -> io::Result<()> {
    let path_ref = path.as_ref();
    debug!("Creating directory: '{}'", path_ref.display());
    match fs::create_dir_all(path_ref) {
        Ok(()) => {
            info!("Successfully created directory: '{}'", path_ref.display());
            Ok(())
        }
        Err(e) => {
            if e.kind() == io::ErrorKind::AlreadyExists {
                warn!("Directory '{}' already exists or cannot be created (check permissions). Assuming success if already exists.", path_ref.display());
                Ok(())
            } else {
                error!("Failed to create directory at path '{}': {}", path_ref.display(), e); 
                Err(e)
            }
        }
    }
}

#[allow(dead_code)]
pub fn read_file(path: impl AsRef<Path>) -> io::Result<String> {
    let path_ref = path.as_ref();
    debug!("Reading file: '{}'", path_ref.display());

    if !path_ref.exists() || !path_ref.is_file() { 
        error!("File does not exist or is not a file: '{}'", path_ref.display());
        return Err(io::Error::new(io::ErrorKind::NotFound, format!("File not found: {}", path_ref.display())));
    }

    match fs::read_to_string(path_ref) {
        Ok(content) => {
            debug!("Read file: '{}', size: {} bytes", path_ref.display(), content.len());
            Ok(content)
        }
        Err(e) => {
            error!("Failed to read file at path '{}': {}", path_ref.display(), e);
            Err(e)
        }
    }
}

#[allow(dead_code)]
#[deprecated(note = "Please use read_file instead")]
pub fn read_file_to_string(path: impl AsRef<Path>) -> io::Result<String> {
    read_file(path)
}

#[allow(dead_code)]
pub fn write_file(path: impl AsRef<Path>, content: &str) -> io::Result<()> {
    let path_ref = path.as_ref();
    debug!("Writing file: '{}', size: {} bytes", path_ref.display(), content.len());

    if let Some(parent_dir) = path_ref.parent() {
        if !parent_dir.as_os_str().is_empty() && !(parent_dir.exists() && parent_dir.is_dir()) { 
            info!("Parent directory '{}' does not exist. Creating.", parent_dir.display());
            create_directory(parent_dir)?;
        }
    }

    match fs::write(path_ref, content) {
        Ok(()) => {
            debug!("Successfully written file: '{}'", path_ref.display());
            Ok(())
        }
        Err(e) => {
            error!("Failed to write file at path '{}': {}", path_ref.display(), e);
            Err(e)
        }
    }
}

#[allow(dead_code)]
pub fn get_file_size(path: impl AsRef<Path>) -> io::Result<u64> {
    let path_ref = path.as_ref();
    if !(path_ref.exists() && path_ref.is_file()) { 
        warn!("Cannot get size of non-existent or non-file: '{}'", path_ref.display());
        return Err(io::Error::new(io::ErrorKind::NotFound, format!("File not found: {}", path_ref.display())));
    }
    match fs::metadata(path_ref) {
        Ok(metadata) => {
            let size = metadata.len();
            debug!("File size: '{}', {} bytes", path_ref.display(), size);
            Ok(size)
        }
        Err(e) => {
            error!("Failed to get file size at path '{}': {}", path_ref.display(), e);
            Err(e)
        }
    }
}

#[allow(dead_code)]
pub fn get_file_extension(path: impl AsRef<Path>) -> Option<String> {
    let path_ref = path.as_ref();
    match path_ref.extension().and_then(|os_str| os_str.to_str()) {
        Some(ext_str) if !ext_str.is_empty() => {
            debug!("File extension for '{}' is '{}'", path_ref.display(), ext_str);
            Some(ext_str.to_string())
        }
        _ => {
            warn!("Could not get file extension for: '{}'", path_ref.display());
            None
        }
    }
}

#[allow(dead_code)]
pub fn get_file_name(path: impl AsRef<Path>) -> Option<String> {
    let path_ref = path.as_ref();
    match path_ref.file_stem().and_then(|os_str| os_str.to_str()) {
        Some(name) => {
            debug!("File name (stem) for '{}' is '{}'", path_ref.display(), name);
            Some(name.to_string())
        }
        None => {
            warn!("Could not get file name (stem) for: '{}'", path_ref.display());
            None
        }
    }
}

#[allow(dead_code)]
pub fn get_directory_name(path: impl AsRef<Path>) -> Option<String> {
    let path_ref = path.as_ref();
    match path_ref.parent() {
        Some(parent_path) => {
            let dir_str = parent_path.to_str().unwrap_or("");
            if dir_str.is_empty() {
                if !path_ref.is_absolute() && path_ref.components().count() == 1 {
                    debug!("Directory for relative file '{}' is current directory '.'", path_ref.display());
                    Some(".".to_string())
                } else if parent_path.as_os_str().is_empty() && !path_ref.is_absolute() {
                     debug!("Directory for relative file '{}' (empty parent) is current directory '.'", path_ref.display());
                    Some(".".to_string())
                }
                 else { 
                    debug!("Directory name for '{}' is '{}'", path_ref.display(), dir_str);
                    Some(dir_str.to_string())
                }
            } else {
                debug!("Directory name for '{}' is '{}'", path_ref.display(), dir_str);
                Some(dir_str.to_string())
            }
        }
        None => { 
            if path_ref.as_os_str().is_empty() {
                warn!("Cannot get directory name for an empty path.");
                None
            } else if path_ref.is_absolute() { 
                debug!("Path '{}' is a root path, its directory is itself.", path_ref.display());
                path_ref.to_str().map(String::from)
            } else {
                 warn!("Could not determine directory name for path: '{}'", path_ref.display());
                 None
            }
        }
    }
}

#[allow(dead_code)]
pub fn list_files(
    dir_path: impl AsRef<Path>,
    pattern: Option<&str>, 
    recursive: bool,
) -> io::Result<Vec<PathBuf>> {
    let dir_path_ref = dir_path.as_ref();
    debug!(
        "Listing files: Directory: '{}', Pattern: {:?}, Recursive: {}",
        dir_path_ref.display(),
        pattern.unwrap_or("<无>"),
        recursive
    );

    if !(dir_path_ref.exists() && dir_path_ref.is_dir()) { 
        warn!("Directory does not exist or is not a directory: '{}'", dir_path_ref.display());
        return Ok(Vec::new()); 
    }

    let pattern_regex = match pattern {
        Some(p_str) if !p_str.is_empty() => {
            match Regex::new(p_str) {
                Ok(re) => Some(re),
                Err(e) => {
                    error!("Invalid regex pattern '{}': {}", p_str, e);
                    return Err(io::Error::new(io::ErrorKind::InvalidInput, format!("Invalid regex pattern: {}", e)));
                }
            }
        },
        _ => None, 
    };

    let mut files = Vec::new();
    let walker = WalkDir::new(dir_path_ref).max_depth(if recursive { usize::MAX } else { 1 });

    for entry_result in walker.into_iter() {
        match entry_result {
            Ok(entry) => {
                if entry.file_type().is_file() {
                    let file_path = entry.path();
                    if let Some(re) = &pattern_regex {
                        if file_path.to_str().map_or(false, |s| re.is_match(s)) {
                            files.push(file_path.to_path_buf());
                        }
                    } else { 
                        files.push(file_path.to_path_buf());
                    }
                }
            }
            Err(e) => {
                warn!("Error accessing entry in directory '{}': {}", dir_path_ref.display(), e);
            }
        }
    }
    debug!("Found {} files in '{}' (Pattern: {:?}, Recursive: {})", files.len(), dir_path_ref.display(), pattern.unwrap_or("N/A"), recursive);
    Ok(files)
}

#[allow(dead_code)]
pub fn list_files_with_filter(
    dir_path: impl AsRef<Path>,
    filter: &dyn Fn(&Path) -> bool, 
    pattern: Option<&str>,
    recursive: bool,
) -> io::Result<Vec<PathBuf>> {
    let dir_path_ref = dir_path.as_ref();
     debug!(
        "Listing files with filter: Directory: '{}', Pattern: {:?}, Recursive: {}",
        dir_path_ref.display(),
        pattern.unwrap_or("<无>"),
        recursive
    );
    
    let base_files = list_files(dir_path_ref, pattern, recursive)?;
    let filtered_files: Vec<PathBuf> = base_files.into_iter().filter(|path_buf| filter(path_buf.as_path())).collect();
    
    debug!("Found {} files matching filter in '{}'", filtered_files.len(), dir_path_ref.display());
    Ok(filtered_files)
}


#[allow(dead_code)]
pub fn join_paths(path1: impl AsRef<Path>, path2: impl AsRef<Path>) -> PathBuf {
    let p1 = path1.as_ref();
    let p2 = path2.as_ref();
    let joined = p1.join(p2);
    debug!("Joining paths: '{}' + '{}' = '{}'", p1.display(), p2.display(), joined.display());
    joined
}

#[allow(dead_code)]
pub fn has_extension(file_path: impl AsRef<Path>, extension_with_dot: &str) -> bool {
    let path_ref = file_path.as_ref();
    
    if !extension_with_dot.starts_with('.') || extension_with_dot.len() == 1 {
        warn!("Invalid target extension format: '{}'. Must start with '.' and be non-empty e.g. '.txt'", extension_with_dot);
        return false;
    }
    let clean_target_ext = &extension_with_dot[1..];

    let result = path_ref.extension()
        .and_then(|os_str| os_str.to_str())
        .map_or(false, |file_ext_str| file_ext_str.eq_ignore_ascii_case(clean_target_ext));
    
    debug!(
        "Checking if '{}' has extension '{}': {}",
        path_ref.display(),
        extension_with_dot,
        result
    );
    result
}

#[allow(dead_code)]
fn generate_random_filename(base_prefix: &str, suffix: &str) -> String {
    let random_part: String = rand::thread_rng()
        .sample_iter(&rand::distributions::Alphanumeric)
        .take(10) 
        .map(char::from)
        .collect();
    format!("{}{}{}", base_prefix, random_part, suffix)
}

#[allow(dead_code)]
pub fn create_temp_file_with_content(content: &str) -> io::Result<PathBuf> {
    debug!("Creating temp file with content, size: {} bytes", content.len());
    let temp_dir = std::env::temp_dir();
    let file_name = generate_random_filename("dlogcover_temp_", ".tmp");
    let temp_file_path = temp_dir.join(file_name);

    write_file(&temp_file_path, content)?;

    if let Ok(mut guard) = TEMP_FILES_TO_CLEANUP.lock() {
        guard.push(temp_file_path.clone());
        info!("Created temp file '{}' with content, scheduled for cleanup.", temp_file_path.display());
    } else {
        error!("Failed to lock TEMP_FILES_TO_CLEANUP to add: {}", temp_file_path.display());
    }
    Ok(temp_file_path)
}

#[allow(dead_code)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TempFileType {
    Empty,
    WithContent, 
}

#[allow(dead_code)]
pub fn create_temp_file(prefix: &str, file_type: TempFileType) -> io::Result<PathBuf> {
    debug!("Creating temp file, prefix: '{}', type: {:?}", prefix, file_type);
    let temp_dir = std::env::temp_dir();
    let file_name_prefix = format!("{}_", prefix);
    let file_name = generate_random_filename(&file_name_prefix, ".tmp");
    let temp_file_path = temp_dir.join(file_name);

    match file_type {
        TempFileType::Empty => {
            File::create(&temp_file_path)?; 
            debug!("Created empty temp file: '{}'", temp_file_path.display());
        }
        TempFileType::WithContent => {
            write_file(&temp_file_path, "This is a temporary file created by DLogCover.")?;
            debug!("Created temp file with default content: '{}'", temp_file_path.display());
        }
    }

    if let Ok(mut guard) = TEMP_FILES_TO_CLEANUP.lock() {
        guard.push(temp_file_path.clone());
        info!("Created temp file '{}' ({:?}), scheduled for cleanup.", temp_file_path.display(), file_type);
    } else {
        error!("Failed to lock TEMP_FILES_TO_CLEANUP to add: {}", temp_file_path.display());
    }
    Ok(temp_file_path)
}

#[allow(dead_code)]
pub fn cleanup_temp_files() {
    if let Ok(mut guard) = TEMP_FILES_TO_CLEANUP.lock() {
        if guard.is_empty() {
            debug!("No temp files registered for cleanup.");
            return;
        }
        info!("Starting cleanup of {} registered temp files.", guard.len());
        let mut remaining_files = Vec::new(); 
        for file_path in guard.drain(..) { 
            match fs::remove_file(&file_path) {
                Ok(_) => debug!("Successfully deleted temp file: '{}'", file_path.display()),
                Err(e) => {
                    warn!("Failed to delete temp file '{}': {}", file_path.display(), e);
                    remaining_files.push(file_path); 
                }
            }
        }
        *guard = remaining_files; 
        if guard.is_empty() {
            info!("All registered temp files cleaned up successfully.");
        } else {
            warn!("{} temp files could not be cleaned up and remain in tracking list.", guard.len());
        }
    } else {
        error!("Failed to acquire lock for temp file cleanup. Cleanup skipped.");
    }
}

#[allow(dead_code)]
pub fn normalize_path(path: impl AsRef<Path>) -> PathBuf {
    let path_ref = path.as_ref();
    if path_ref.as_os_str().is_empty() {
        return PathBuf::new();
    }

    let mut components = Vec::new();
    for component in path_ref.components() {
        match component {
            std::path::Component::ParentDir => {
                if let Some(std::path::Component::Normal(_)) = components.last() {
                    components.pop();
                } else if path_ref.is_absolute() && components.last() == Some(&std::path::Component::RootDir) {
                    // No change
                } else { 
                    components.push(component);
                }
            }
            std::path::Component::CurDir => {
                // Ignore
            }
            _ => { 
                components.push(component);
            }
        }
    }

    let mut result = PathBuf::new();
    if components.is_empty() {
        if path_ref.is_absolute() {
            if let Some(std::path::Component::RootDir) = path_ref.components().next() {
                 result.push(std::path::Component::RootDir.as_os_str());
            } else { 
                result = PathBuf::from("."); 
            }
        } else {
            result = PathBuf::from(".");
        }
    } else {
        if components.len() == 1 && components[0] == std::path::Component::RootDir && path_ref.as_os_str() != "/" {
             result.push(components[0].as_os_str());
        } else {
            for (idx, comp) in components.iter().enumerate() {
                 if idx > 0 && comp == &std::path::Component::RootDir && result.ends_with("/") {
                     continue;
                 }
                result.push(comp.as_os_str());
            }
        }
    }
    if path_ref.as_os_str() == "." && result.as_os_str() != "." { result = PathBuf::from("."); }
    if path_ref.as_os_str() == ".." && result.as_os_str() != ".." { result = PathBuf::from(".."); }


    debug!("Normalized path: '{}' -> '{}'", path_ref.display(), result.display());
    result
}

#[allow(dead_code)]
pub fn get_relative_path(path: impl AsRef<Path>, base: impl AsRef<Path>) -> io::Result<PathBuf> {
    let original_path_display = path.as_ref().display().to_string(); 
    let original_base_display = base.as_ref().display().to_string(); 

    let abs_path = match to_absolute_path(path.as_ref()) { 
        Ok(p) => p,
        Err(e) => {
            error!("get_relative_path (making path absolute) for '{}': {}", &original_path_display, e);
            return Err(e);
        }
    };
    let abs_base = match to_absolute_path(base.as_ref()) { 
        Ok(p) => p,
        Err(e) => {
            error!("get_relative_path (making base absolute) for '{}': {}", &original_base_display, e);
            return Err(e);
        }
    };

    if abs_path == abs_base {
        debug!("Paths '{}' and '{}' are identical. Relative path is '.'", abs_path.display(), abs_base.display());
        return Ok(PathBuf::from("."));
    }
    
    if let Ok(stripped) = abs_path.strip_prefix(&abs_base) {
         debug!("Relative path of '{}' to '{}' is '{}'", abs_path.display(), abs_base.display(), stripped.display());
        return Ok(stripped.to_path_buf());
    }

    let mut path_comps_iter = abs_path.components().peekable();
    let mut base_comps_iter = abs_base.components().peekable();

    while let (Some(pc), Some(bc)) = (path_comps_iter.peek(), base_comps_iter.peek()) {
        if pc == bc {
            path_comps_iter.next();
            base_comps_iter.next();
        } else {
            break;
        }
    }

    let mut result = PathBuf::new();
    for _ in base_comps_iter {
        result.push("..");
    }
    for comp in path_comps_iter {
        result.push(comp.as_os_str());
    }
    
    if result.as_os_str().is_empty() {
        debug!("Resulting relative path for '{}' and '{}' is empty, defaulting to '.'", abs_path.display(), abs_base.display());
        Ok(PathBuf::from("."))
    } else {
        debug!("Calculated relative path of '{}' to '{}' is '{}'", abs_path.display(), abs_base.display(), result.display());
        Ok(result)
    }
}


#[allow(dead_code)]
pub fn create_directory_if_not_exists(path: impl AsRef<Path>) -> io::Result<()> {
    let path_ref = path.as_ref();
    if path_ref.exists() && path_ref.is_dir() { // Corrected: removed unnecessary parentheses
        debug!("Directory '{}' already exists. No action needed.", path_ref.display());
        return Ok(());
    }
    debug!("Directory '{}' does not exist. Attempting to create.", path_ref.display());
    create_directory(path_ref) 
}

#[allow(dead_code)]
pub fn to_absolute_path(path: impl AsRef<Path>) -> io::Result<PathBuf> {
    let path_ref = path.as_ref();
    let result_path = if path_ref.is_absolute() {
        path_ref.to_path_buf()
    } else {
        std::env::current_dir()?.join(path_ref)
    };
    Ok(normalize_path(result_path))
}
