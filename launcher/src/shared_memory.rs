use crate::error::{AppError, Result};
use std::{
    ffi::CString,
    ptr::{self, copy_nonoverlapping},
};
use windows_sys::Win32::{
    Foundation::{CloseHandle, HANDLE},
    System::Memory::{
        CreateFileMappingA, FILE_MAP_WRITE, MapViewOfFile, PAGE_READWRITE, UnmapViewOfFile,
    },
};

#[repr(C)]
struct Data {
    size: usize,
    value: [u8; 0],
}

pub fn write_shared_string(name: &str, value: &str) -> Result<()> {
    crate::launcher_trace!(
        "windows::write_shared_string name=\"{}\" value_len={} value=\"{}\"",
        name,
        value.len(),
        value,
    );
    let size = std::mem::size_of::<Data>() + value.len();
    let c_name =
        CString::new(name).map_err(|_| AppError::Message("invalid mapping name".to_string()))?;

    let mapping: HANDLE = unsafe {
        CreateFileMappingA(
            -1isize as HANDLE,
            ptr::null_mut(),
            PAGE_READWRITE,
            0,
            size as u32,
            c_name.as_ptr() as *const u8,
        )
    };

    if mapping.is_null() {
        return Err(AppError::Message("CreateFileMappingA failed".to_string()));
    }

    let view = unsafe { MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, size) };
    if view.Value.is_null() {
        unsafe { CloseHandle(mapping) };
        return Err(AppError::Message("MapViewOfFile failed".to_string()));
    }

    let header = view.Value as *mut Data;
    unsafe {
        (*header).size = value.len();
    }

    let data_ptr = unsafe { (*header).value.as_ptr() as *mut u8 };
    unsafe { copy_nonoverlapping(value.as_ptr(), data_ptr, value.len()) };

    let written =
        unsafe { std::str::from_utf8_unchecked(std::slice::from_raw_parts(data_ptr, value.len())) };

    crate::launcher_trace!("windows::write_shared_string written value=\"{}\"", written,);

    unsafe {
        UnmapViewOfFile(view);
    }

    Ok(())
}
