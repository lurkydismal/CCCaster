use crate::api::{Handle, make_patch, remove_patch};
use serde::Deserialize;

/// Represents a patch entry from patch.json
#[derive(Deserialize)]
pub struct PatchEntry {
    pub address: usize,
    pub bytes: Vec<u8>,
}

/// Manages a set of patches; applies patches on creation and removes them on Drop.
pub struct Patches {
    handles: Vec<Handle>,
}

impl Patches {
    pub fn new(entries: &[PatchEntry]) -> Self {
        let mut handles = Vec::new();
        for entry in entries {
            // Apply patch by calling into the provided API
            let handle = make_patch(entry.address, &entry.bytes);
            handles.push(handle);
        }
        Patches { handles }
    }
}

impl Drop for Patches {
    fn drop(&mut self) {
        // Remove all patches on drop
        for &handle in &self.handles {
            let _ = remove_patch(handle);
        }
    }
}
