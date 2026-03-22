use std::{ffi::CString, ptr::copy_nonoverlapping, ptr::null_mut};

use windows_sys::Win32::{
    Foundation::{CloseHandle, HANDLE},
    System::Memory::{
        CreateFileMappingA, FILE_MAP_WRITE, MapViewOfFile, PAGE_READWRITE, UnmapViewOfFile,
    },
};

#[derive(serde::Serialize)]
pub struct TestData {
    pub name: String,
    pub path: String,
}

#[repr(C)]
struct MyData {
    size: usize,
    value: [u8; 0],
}

pub fn write_shared_string(name: &str, value: &str) -> Result<(), &'static str> {
    let size = std::mem::size_of::<MyData>() + value.len();
    let c_name = CString::new(name).map_err(|_| "invalid mapping name")?;

    let mapping: HANDLE = unsafe {
        CreateFileMappingA(
            -1isize as HANDLE,
            null_mut(),
            PAGE_READWRITE,
            0,
            size as u32,
            c_name.as_ptr() as *const u8,
        )
    };

    if mapping.is_null() {
        return Err("CreateFileMappingA failed");
    }

    let view = unsafe { MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, size) };
    if view.Value.is_null() {
        unsafe { CloseHandle(mapping) };
        return Err("MapViewOfFile failed");
    }

    let header = view.Value as *mut MyData;
    unsafe {
        (*header).size = value.len();
    }

    let data_ptr = unsafe { (*header).value.as_ptr() as *mut u8 };
    unsafe { copy_nonoverlapping(value.as_ptr(), data_ptr, value.len()) };

    unsafe {
        UnmapViewOfFile(view);
        CloseHandle(mapping);
    }

    Ok(())
}
