use std::ptr;
use windows_sys::Win32::{
    Foundation::{GetLastError, HLOCAL, LocalFree},
    System::{
        Diagnostics::Debug::{
            FORMAT_MESSAGE_ALLOCATE_BUFFER, FORMAT_MESSAGE_FROM_SYSTEM,
            FORMAT_MESSAGE_IGNORE_INSERTS, FormatMessageA,
        },
        Performance::{QueryPerformanceCounter, QueryPerformanceFrequency},
    },
};

use crate::{launcher_info, launcher_trace};

pub fn last_error_message(error: Option<u32>) -> String {
    launcher_trace!("last_error_message accepted args: error={error:?}");
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

pub fn measure_block<F, R>(f: F) -> R
where
    F: FnOnce() -> R,
{
    launcher_trace!("measure_block accepted args");
    let mut l_freq = 0i64;
    unsafe { QueryPerformanceFrequency(&mut l_freq) };

    let mut l_start = 0i64;
    let mut l_end = 0i64;

    unsafe { QueryPerformanceCounter(&mut l_start) };

    let l_result = f();

    unsafe { QueryPerformanceCounter(&mut l_end) };

    let l_seconds = (l_end - l_start) as f64 / l_freq as f64;
    let l_seconds = l_seconds * 1000.0; // ms

    launcher_info!("Measured load/ injection phases: {l_seconds} milliseconds");

    l_result
}
