use std::ptr;

use windows_sys::Win32::{
    Foundation::{GetLastError, HLOCAL, LocalFree},
    System::Diagnostics::Debug::{
        FORMAT_MESSAGE_ALLOCATE_BUFFER, FORMAT_MESSAGE_FROM_SYSTEM, FORMAT_MESSAGE_IGNORE_INSERTS,
        FormatMessageA,
    },
};

pub fn last_error_message(error: Option<u32>) -> String {
    let l_error = error.unwrap_or_else(|| unsafe { GetLastError() });

    let mut l_buffer: *mut u8 = ptr::null_mut();

    let l_size = unsafe {
        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM
                | FORMAT_MESSAGE_IGNORE_INSERTS
                | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            ptr::null_mut(),
            l_error,
            0,
            (&mut l_buffer) as *mut _ as *mut u8,
            0,
            ptr::null_mut(),
        )
    };

    if l_size == 0 || l_buffer.is_null() {
        return format!("Unknown error ({})", l_error);
    }

    let l_slice = unsafe { std::slice::from_raw_parts(l_buffer, l_size as usize) };
    let mut l_string = String::from_utf8_lossy(l_slice).into_owned();

    while l_string.ends_with(['\r', '\n']) {
        l_string.pop();
    }

    unsafe { LocalFree(l_buffer as HLOCAL) };

    format!("{} ({})", l_string, l_error)
}
