use std::{os::raw::c_char, sync::OnceLock};

use crate::{modloader_debug, modloader_trace};

pub mod engine_variables;
pub mod init_bridge;

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
    static ref API: OnceLock<Api> = OnceLock::new();
}

pub(crate) fn set_api_from_ptr(vtable: *const Api) -> bool {
    if vtable.is_null() {
        return false;
    }
    // SAFETY: `vtable` points to a C ABI table validated by the caller.
    let api = unsafe { std::ptr::read(vtable) };
    API.set(api).ok();
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
    // SAFETY: the API table is initialized during `init` before patch operations.
    unsafe { make_patch_raw(addr, bytes.as_ptr(), bytes.len()) }
}

/// Safe wrapper around the raw FFI patch-removal callback.
pub fn remove_patch(id: Handle) -> bool {
    modloader_trace!("remove_patch requested: handle={}", id);
    // SAFETY: the API table is initialized during `init` before patch operations.
    unsafe { remove_patch_raw(id) }
}

/// Safe wrapper around the raw FFI memory-read callback.
pub fn read_memory(addr: usize, len: usize) -> Option<Vec<u8>> {
    if len == 0 {
        return Some(Vec::new());
    }

    let mut out = vec![0u8; len];
    // SAFETY: `out` is valid for writes of `len` bytes.
    let ok = unsafe { read_memory_raw(addr, out.as_mut_ptr(), out.len()) };
    ok.then_some(out)
}

/// Safe wrapper around the raw FFI memory-write callback.
pub fn write_memory(addr: usize, bytes: &[u8]) -> bool {
    if bytes.is_empty() {
        return false;
    }
    // SAFETY: `bytes` is valid for reads of `bytes.len()` bytes.
    unsafe { write_memory_raw(addr, bytes.as_ptr(), bytes.len()) }
}

/// # Safety
/// Requires that `API` has already been initialized and pointers are valid.
unsafe fn make_patch_raw(addr: usize, bytes: *const u8, len: usize) -> Handle {
    let api = API.get().expect("API not initialized");

    // SAFETY: callback pointer and argument contract come from host process.
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

    // SAFETY: callback pointer and argument contract come from host process.
    let removed = unsafe { (api.remove_patch)(id) };
    modloader_debug!("remove_patch_raw handle={} removed={}", id, removed);
    removed
}

/// # Safety
/// Requires that `API` has already been initialized and pointers are valid.
unsafe fn read_memory_raw(addr: usize, out: *mut u8, len: usize) -> bool {
    let api = API.get().expect("API not initialized");
    // SAFETY: callback pointer and argument contract come from host process.
    unsafe { (api.read_memory)(addr, out, len) }
}

/// # Safety
/// Requires that `API` has already been initialized and pointers are valid.
unsafe fn write_memory_raw(addr: usize, bytes: *const u8, len: usize) -> bool {
    let api = API.get().expect("API not initialized");
    // SAFETY: callback pointer and argument contract come from host process.
    unsafe { (api.write_memory)(addr, bytes, len) }
}
