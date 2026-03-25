use std::path::{Path, PathBuf};

use crate::{AppError, args::Args};

pub fn resolve_path(
    default_name: &str,
    r#override: Option<&Path>,
    root_path: &Path,
) -> Result<PathBuf, AppError> {
    if let Some(l_path) = r#override {
        return Ok(l_path.to_path_buf());
    }

    Ok(root_path.join(default_name))
}

pub fn validate_launcher_inputs(
    exe_path: &Path,
    wrapper_path: &Path,
    addons_path: &Path,
    args: &Args,
) -> Result<(), AppError> {
    if !exe_path.exists() {
        return Err(AppError::Message(format!(
            "game executable not found: {}",
            exe_path.display()
        )));
    }

    if !args.no_inject {
        if !wrapper_path.exists() {
            return Err(AppError::Message(format!(
                "wrapper dll not found: {}",
                wrapper_path.display()
            )));
        }

        if !addons_path.exists() {
            return Err(AppError::Message(format!(
                "addons directory not found: {}",
                addons_path.display()
            )));
        }
    }

    Ok(())
}
