/// Shared path normalization, hashing, and safety helpers for loader internals.
use blake3::Hash;
use std::path::{Path, PathBuf};

/// Sanitizes path segments for nested table export field names.
pub(super) fn sanitize_identifier(name: &str) -> String {
    name.replace(' ', "_")
}

/// Attempts to canonicalize a path; falls back to original path on failure.
pub(super) fn normalize_path(path: &Path) -> PathBuf {
    path.canonicalize().unwrap_or_else(|_| path.to_path_buf())
}

/// Hashes file bytes for content-based hot-reload detection.
pub(super) fn hash_file(path: &Path) -> Option<Hash> {
    match std::fs::read(path) {
        Ok(data) => Some(blake3::hash(&data)),
        Err(_) => None,
    }
}

/// Ensures an addon path is contained within the configured addons base.
pub(super) fn is_safe_path(base: &Path, child: &Path) -> bool {
    match (base.canonicalize(), child.canonicalize()) {
        (Ok(b), Ok(c)) => c.starts_with(&b),
        _ => false,
    }
}
