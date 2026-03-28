use once_cell::sync::OnceLock;
use std::os::raw::c_char;

pub type Handle = u32;
pub type MakePatchFn = unsafe extern "C" fn(addr: usize, bytes: *const u8, len: usize) -> Handle;
pub type RemovePatchFn = unsafe extern "C" fn(id: Handle) -> bool;

pub struct Api {
    pub make_patch: MakePatchFn,
    pub remove_patch: RemovePatchFn,
}

static API: OnceLock<Api> = OnceLock::new();

#[no_mangle]
pub extern "C" fn init(
    make_patch: MakePatchFn,
    remove_patch: RemovePatchFn,
    json: *const c_char,
    json_len: usize,
) -> bool {
    // Initialize the API function pointers
    API.set(Api {
        make_patch,
        remove_patch,
    })
    .ok();

    // If JSON config is provided, it could be parsed here
    if json.is_null() {
        return false;
    }

    let slice = unsafe { std::slice::from_raw_parts(json as *const u8, json_len) };
    let _ = std::str::from_utf8(slice);

    true
}

pub fn make_patch(addr: usize, bytes: &[u8]) -> Handle {
    unsafe { make_patch_raw(addr, bytes.as_ptr(), bytes.len()) }
}

pub fn remove_patch(id: Handle) -> bool {
    unsafe { remove_patch_raw(id) }
}

/// # Safety
unsafe fn make_patch_raw(addr: usize, bytes: *const u8, len: usize) -> Handle {
    let api = API.get().expect("API not initialized");
    (api.make_patch)(addr, bytes, len)
}

/// # Safety
unsafe fn remove_patch_raw(id: Handle) -> bool {
    let api = API.get().expect("API not initialized");
    (api.remove_patch)(id)
}
