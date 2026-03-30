use std::os::raw::c_char;

use crate::{
    main, modloader_debug, modloader_error, modloader_info, modloader_trace, modloader_warning,
    parse_args,
};

pub type Handle = u32;

/// FFI callback for creating a patch in the host process.
pub type MakePatchFn = unsafe extern "C" fn(addr: usize, bytes: *const u8, len: usize) -> Handle;

/// FFI callback for removing a previously created patch.
pub type RemovePatchFn = unsafe extern "C" fn(id: Handle) -> bool;

/// Immutable API table provided by the host at initialization time.
pub struct Api {
    pub make_patch: MakePatchFn,
    pub remove_patch: RemovePatchFn,
}

lazy_static::lazy_static! {
    static ref API: std::sync::OnceLock<Api> = std::sync::OnceLock::new();
}

#[unsafe(no_mangle)]
pub extern "C" fn init(
    make_patch: MakePatchFn,
    remove_patch: RemovePatchFn,
    json: *const c_char,
    json_len: usize,
) -> bool {
    modloader_info!("Initializing C API bridge");
    API.set(Api {
        make_patch,
        remove_patch,
    })
    .ok();
    modloader_debug!("Registered patch API function pointers");

    // Convert JSON if needed
    if json.is_null() {
        modloader_error!("init received null json pointer");
        return false;
    }

    let slice = unsafe { std::slice::from_raw_parts(json as *const u8, json_len) };
    modloader_trace!("Received init JSON payload with {} bytes", json_len);
    match std::str::from_utf8(slice) {
        Ok(json_str) => {
            modloader_debug!(
                "init JSON payload validated as UTF-8 ({} chars)",
                json_str.len()
            );
            let preview = json_str.chars().take(120).collect::<String>();
            modloader_trace!("init JSON preview (first 120 chars): {}", preview);

            if let Err(err) = parse_args(json_str) {
                modloader_error!("Failed to parse modloader init arguments JSON: {err}");

                return false;
            }
        }
        Err(err) => {
            modloader_warning!("init JSON payload is not valid UTF-8: {}", err);

            return false;
        }
    }

    modloader_info!("Calling async modloader main entrypoint");

    if let Err(err) = main() {
        modloader_error!("Modloader startup failed: {}", err);

        for (i, cause) in err.chain().skip(1).enumerate() {
            modloader_error!("Root cause [{}]: {}", i, cause);
        }
    } else {
        modloader_info!("Modloader main entrypoint completed successfully");
    }

    true
}

/// Safe wrapper around the raw FFI patch-creation callback.
pub fn make_patch(addr: usize, bytes: &[u8]) -> Handle {
    modloader_trace!(
        "make_patch requested: addr=0x{:X}, len={}, bytes_preview={:02X?}",
        addr,
        bytes.len(),
        bytes.iter().take(8).copied().collect::<Vec<u8>>()
    );
    unsafe { make_patch_raw(addr, bytes.as_ptr(), bytes.len()) }
}

/// Safe wrapper around the raw FFI patch-removal callback.
pub fn remove_patch(id: Handle) -> bool {
    modloader_trace!("remove_patch requested: handle={}", id);
    unsafe { remove_patch_raw(id) }
}

/// # Safety
/// Requires that `API` has already been initialized and pointers are valid.
unsafe fn make_patch_raw(addr: usize, bytes: *const u8, len: usize) -> Handle {
    let api = API.get().expect("API not initialized");

    let handle = unsafe { (api.make_patch)(addr, bytes, len) };
    modloader_debug!(
        "make_patch_raw applied patch at 0x{:X} with len={} => handle={}",
        addr,
        len,
        handle
    );
    handle
}

/// # Safety
/// Requires that `API` has already been initialized and pointers are valid.
unsafe fn remove_patch_raw(id: Handle) -> bool {
    let api = API.get().expect("API not initialized");

    let removed = unsafe { (api.remove_patch)(id) };
    modloader_debug!("remove_patch_raw handle={} removed={}", id, removed);
    removed
}
