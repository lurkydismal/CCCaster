use std::{path::Path, ptr};

use windows_sys::Win32::{
    Foundation::{CloseHandle, HANDLE, WAIT_OBJECT_0, WAIT_TIMEOUT},
    System::{
        Diagnostics::Debug::WriteProcessMemory,
        LibraryLoader::LoadLibraryW,
        Memory::{
            MEM_COMMIT, MEM_RELEASE, MEM_RESERVE, PAGE_READWRITE, VirtualAllocEx, VirtualFreeEx,
        },
        Threading::{CreateRemoteThread, GetExitCodeThread, WaitForSingleObject},
    },
};

use crate::{
    AppError,
    launcher::windows::{process, win32},
};

pub fn inject_dll(
    h_process: HANDLE, wrapper_path: &Path, timeout_ms: Option<usize>, wait_for_wrapper: bool,
) -> Result<(), AppError> {
    let l_wrapper_wide = process::to_wide_null(wrapper_path.as_os_str());
    let l_bytes = l_wrapper_wide.len() * size_of::<u16>();

    let l_remote_mem = unsafe {
        VirtualAllocEx(
            h_process,
            ptr::null_mut(),
            l_bytes,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE,
        )
    };

    if l_remote_mem.is_null() {
        return Err(AppError::Message(format!(
            "VirtualAllocEx failed: {}",
            win32::last_error_message(None)
        )));
    }

    let l_write_ok = unsafe {
        WriteProcessMemory(
            h_process,
            l_remote_mem,
            l_wrapper_wide.as_ptr() as *const _,
            l_bytes,
            ptr::null_mut(),
        )
    };

    if l_write_ok == 0 {
        unsafe { VirtualFreeEx(h_process, l_remote_mem, 0, MEM_RELEASE) };
        return Err(AppError::Message(format!(
            "WriteProcessMemory failed: {}",
            win32::last_error_message(None)
        )));
    }

    let l_start_routine: windows_sys::Win32::System::Threading::LPTHREAD_START_ROUTINE =
        Some(unsafe { std::mem::transmute(LoadLibraryW as *const () as usize) });

    let l_thread = unsafe {
        CreateRemoteThread(
            h_process,
            ptr::null_mut(),
            0,
            l_start_routine,
            l_remote_mem,
            0,
            ptr::null_mut(),
        )
    };

    if l_thread.is_null() {
        unsafe { VirtualFreeEx(h_process, l_remote_mem, 0, MEM_RELEASE) };
        return Err(AppError::Message(format!(
            "CreateRemoteThread failed: {}",
            win32::last_error_message(None)
        )));
    }

    let mut l_exit_code: u32 = 0;

    if wait_for_wrapper {
        let l_wait_ms = match timeout_ms {
            | Some(l_timeout) => {
                if l_timeout > u32::MAX as usize {
                    unsafe { CloseHandle(l_thread) };
                    unsafe { VirtualFreeEx(h_process, l_remote_mem, 0, MEM_RELEASE) };
                    return Err(AppError::Message("inject_timeout is too large".to_string()));
                }
                l_timeout as u32
            }
            | None => u32::MAX,
        };

        let l_wait_result = unsafe { WaitForSingleObject(l_thread, l_wait_ms) };
        if l_wait_result == WAIT_TIMEOUT {
            unsafe { CloseHandle(l_thread) };
            unsafe { VirtualFreeEx(h_process, l_remote_mem, 0, MEM_RELEASE) };
            return Err(AppError::Message("wrapper injection timed out".to_string()));
        }

        if l_wait_result != WAIT_OBJECT_0 {
            unsafe { CloseHandle(l_thread) };
            unsafe { VirtualFreeEx(h_process, l_remote_mem, 0, MEM_RELEASE) };
            return Err(AppError::Message(format!(
                "WaitForSingleObject failed: {}",
                win32::last_error_message(None)
            )));
        }

        if unsafe { GetExitCodeThread(l_thread, &mut l_exit_code) } == 0 {
            unsafe { CloseHandle(l_thread) };
            unsafe { VirtualFreeEx(h_process, l_remote_mem, 0, MEM_RELEASE) };
            return Err(AppError::Message(format!(
                "GetExitCodeThread failed: {}",
                win32::last_error_message(None)
            )));
        }
    }

    unsafe { CloseHandle(l_thread) };

    if wait_for_wrapper {
        unsafe { VirtualFreeEx(h_process, l_remote_mem, 0, MEM_RELEASE) };

        if l_exit_code == 0 {
            return Err(AppError::Message("LoadLibraryW failed in remote process".to_string()));
        }
    }

    Ok(())
}
