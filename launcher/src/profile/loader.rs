use crate::error::{AppError, Result};
use std::{
    fs,
    io::{BufReader, BufWriter},
    path::Path,
};

use super::Profile;

pub fn load_profile(profile_path: impl AsRef<Path>) -> Result<Profile> {
    let l_path = profile_path.as_ref();

    let l_file = fs::File::open(l_path).map_err(|l_err| {
        AppError::Message(format!(
            "Failed to open profile '{}': {}",
            l_path.display(),
            l_err
        ))
    })?;

    let l_reader = BufReader::new(l_file);

    serde_json::from_reader(l_reader).map_err(|l_err| {
        AppError::Message(format!(
            "Failed to parse profile '{}': {}",
            l_path.display(),
            l_err
        ))
    })
}

pub fn save_profile(profile_path: impl AsRef<Path>, profile: &Profile) -> Result<()> {
    let l_path = profile_path.as_ref();

    if let Some(l_parent) = l_path.parent() {
        fs::create_dir_all(l_parent).map_err(|l_err| {
            AppError::Message(format!(
                "Failed to create profile directory '{}': {}",
                l_parent.display(),
                l_err
            ))
        })?;
    }

    let l_file = fs::File::create(l_path).map_err(|l_err| {
        AppError::Message(format!(
            "Failed to create profile '{}': {}",
            l_path.display(),
            l_err
        ))
    })?;

    let l_writer = BufWriter::new(l_file);

    serde_json::to_writer_pretty(l_writer, profile).map_err(|l_err| {
        AppError::Message(format!(
            "Failed to write profile '{}': {}",
            l_path.display(),
            l_err
        ))
    })
}
