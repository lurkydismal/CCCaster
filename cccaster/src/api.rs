use std::os::raw::c_char;

use crate::{
    loader, main, modloader_debug, modloader_error, modloader_info, modloader_trace,
    modloader_warning, parse_args,
};

pub type Handle = u32;

/// FFI callback for creating a patch in the host process.
pub type MakePatchFn = unsafe extern "C" fn(addr: usize, bytes: *const u8, len: usize) -> Handle;

/// FFI callback for removing a previously created patch.
pub type RemovePatchFn = unsafe extern "C" fn(id: Handle) -> bool;
/// FFI callback for reading memory from the host process.
pub type ReadMemoryFn = unsafe extern "C" fn(addr: usize, out: *mut u8, len: usize) -> bool;
/// FFI callback for writing memory to the host process.
pub type WriteMemoryFn = unsafe extern "C" fn(addr: usize, bytes: *const u8, len: usize) -> bool;

/// Immutable API table provided by the host at initialization time.
#[repr(C)]
pub struct Api {
    pub make_patch: MakePatchFn,
    pub remove_patch: RemovePatchFn,
    pub read_memory: ReadMemoryFn,
    pub write_memory: WriteMemoryFn,
}

lazy_static::lazy_static! {
    static ref API: std::sync::OnceLock<Api> = std::sync::OnceLock::new();
}

#[ctor::dtor]
fn cccaster_dtor() {
    modloader_info!("cccaster dtor invoked; shutting down addons");
    loader::shutdown_before_unload();
}

#[unsafe(no_mangle)]
pub extern "C" fn init(
    vtable: *const Api,
    json: *const c_char,
    json_len: usize,
) -> bool {
    modloader_info!("Initializing C API bridge");
    if vtable.is_null() {
        modloader_error!("init received null vtable pointer");
        return false;
    }
    // SAFETY: `vtable` is checked for null above and points to an immutable C table.
    let api = unsafe { std::ptr::read(vtable) };

    API.set(api).ok();
    modloader_debug!("Registered API vtable function pointers");

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

/// Safe wrapper around the raw FFI memory-read callback.
pub fn read_memory(addr: usize, len: usize) -> Option<Vec<u8>> {
    if len == 0 {
        return Some(Vec::new());
    }

    let mut out = vec![0u8; len];
    let ok = unsafe { read_memory_raw(addr, out.as_mut_ptr(), out.len()) };
    if ok { Some(out) } else { None }
}

/// Safe wrapper around the raw FFI memory-write callback.
pub fn write_memory(addr: usize, bytes: &[u8]) -> bool {
    if bytes.is_empty() {
        return false;
    }
    unsafe { write_memory_raw(addr, bytes.as_ptr(), bytes.len()) }
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

/// # Safety
/// Requires that `API` has already been initialized and pointers are valid.
unsafe fn read_memory_raw(addr: usize, out: *mut u8, len: usize) -> bool {
    let api = API.get().expect("API not initialized");
    unsafe { (api.read_memory)(addr, out, len) }
}

/// # Safety
/// Requires that `API` has already been initialized and pointers are valid.
unsafe fn write_memory_raw(addr: usize, bytes: *const u8, len: usize) -> bool {
    let api = API.get().expect("API not initialized");
    unsafe { (api.write_memory)(addr, bytes, len) }
}
