use crate::api::{Handle, make_patch, remove_patch};
use crate::{modloader_debug, modloader_trace};
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
        modloader_debug!("Creating Patches container with {} entries", entries.len());
        let mut handles = Vec::new();
        for (idx, entry) in entries.iter().enumerate() {
            modloader_trace!(
                "Applying patch[{}]: address=0x{:X}, bytes={:02X?}",
                idx,
                entry.address,
                entry.bytes
            );
            // Apply patch by calling into the provided API
            let handle = make_patch(entry.address, &entry.bytes);
            modloader_debug!("Patch[{}] applied with handle {}", idx, handle);
            handles.push(handle);
        }
        modloader_debug!("All patch entries applied ({} handles)", handles.len());
        Patches { handles }
    }
}

impl Drop for Patches {
    fn drop(&mut self) {
        // Remove all patches on drop
        modloader_debug!(
            "Dropping Patches container ({} handles)",
            self.handles.len()
        );
        for &handle in &self.handles {
            modloader_trace!("Removing patch handle {}", handle);
            let _ = remove_patch(handle);
        }
        modloader_debug!("Finished removing patch handles");
    }
}
