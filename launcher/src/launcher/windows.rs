use crate::{
    args::Args,
    error::{AppError, Result},
    shared_memory::{TestData, write_shared_string},
};
use std::{
    ffi::OsStr,
    mem::size_of,
    os::windows::ffi::OsStrExt,
    path::{Path, PathBuf},
    ptr::null_mut,
};
use windows_sys::Win32::{
    Foundation::{CloseHandle, GetLastError, HLOCAL, LocalFree, WAIT_OBJECT_0, WAIT_TIMEOUT},
    System::{
        Diagnostics::Debug::{
            FORMAT_MESSAGE_ALLOCATE_BUFFER, FORMAT_MESSAGE_FROM_SYSTEM,
            FORMAT_MESSAGE_IGNORE_INSERTS, FormatMessageA, WriteProcessMemory,
        },
        LibraryLoader::LoadLibraryW,
        Memory::{
            MEM_COMMIT, MEM_RELEASE, MEM_RESERVE, PAGE_READWRITE, VirtualAllocEx, VirtualFreeEx,
        },
        Threading::{
            CREATE_SUSPENDED, CreateProcessW, CreateRemoteThread, GetExitCodeThread,
            PROCESS_INFORMATION, ResumeThread, STARTUPINFOW, WaitForSingleObject,
        },
    },
};

use crate::launcher::{DEFAULT_EXE_NAME, DEFAULT_WRAPPER_NAME};

pub fn run(l_args: &Args) -> Result<()> {
    if l_args.attach {
        return Err(AppError::Message(
            "attach mode is not implemented in this launcher".to_string(),
        ));
    }

    let l_exe_path = resolve_path(DEFAULT_EXE_NAME, None)?;
    let l_wrapper_path = resolve_path(
        DEFAULT_WRAPPER_NAME,
        l_args.wrapper.as_deref().map(Path::new),
    )?;

    if l_args.validate || l_args.dry_run {
        validate_launcher_inputs(&l_exe_path, &l_wrapper_path, l_args)?;
        if l_args.validate {
            println!("validation ok");
        } else {
            println!("dry run ok");
        }
        return Ok(());
    }

    if l_args.no_inject {
        launch_without_injection(&l_exe_path, &l_args.game_args)?;
        return Ok(());
    }

    launch_with_injection(&l_exe_path, &l_wrapper_path, l_args)
}

fn launch_without_injection(exe_path: &Path, game_args: &[String]) -> Result<()> {
    let mut l_pi = spawn_process(exe_path, game_args, false)?;
    println!("process started");
    close_process_handles(&mut l_pi);
    Ok(())
}

fn launch_with_injection(exe_path: &Path, wrapper_path: &Path, l_args: &Args) -> Result<()> {
    let mut l_pi = spawn_process(exe_path, &l_args.game_args, true)?;
    println!("process started suspended");

    let l_data = TestData {
        name: "example".to_string(),
        path: "/tmp/file".to_string(),
    };

    let l_json = serde_json::to_string(&l_data)?;

    if let Err(l_err) = write_shared_string("Local\\MySharedData", &l_json) {
        eprintln!("{l_err}");
    }

    inject_dll(&mut l_pi, wrapper_path, l_args.inject_timeout)?;

    println!("dll injected");

    if l_args.suspend || l_args.break_on_load {
        println!("leaving process suspended");
        close_process_handles(&mut l_pi);
        return Ok(());
    }

    println!("resuming process");
    let l_resume_result = unsafe { ResumeThread(l_pi.hThread) };
    if l_resume_result == u32::MAX {
        let l_err = unsafe { GetLastError() };
        close_process_handles(&mut l_pi);
        return Err(AppError::Message(format!(
            "ResumeThread failed: {}",
            last_error_message(Some(l_err))
        )));
    }

    close_process_handles(&mut l_pi);
    Ok(())
}

fn spawn_process(
    exe_path: &Path,
    game_args: &[String],
    suspended: bool,
) -> Result<PROCESS_INFORMATION> {
    let mut l_si: STARTUPINFOW = unsafe { std::mem::zeroed() };
    let mut l_pi: PROCESS_INFORMATION = unsafe { std::mem::zeroed() };

    l_si.cb = size_of::<STARTUPINFOW>() as u32;

    let l_exe_wide = to_wide_null(exe_path.as_os_str());
    let l_cmdline = build_command_line(exe_path, game_args);
    let mut l_cmdline_wide = to_wide_null(OsStr::new(&l_cmdline));

    let l_flags = if suspended { CREATE_SUSPENDED } else { 0 };

    let l_ok = unsafe {
        CreateProcessW(
            l_exe_wide.as_ptr(),
            l_cmdline_wide.as_mut_ptr(),
            null_mut(),
            null_mut(),
            0,
            l_flags,
            null_mut(),
            null_mut(),
            &mut l_si,
            &mut l_pi,
        )
    };

    if l_ok == 0 {
        return Err(AppError::Message(format!(
            "CreateProcessW failed: {}",
            last_error_message(None)
        )));
    }

    Ok(l_pi)
}

fn build_command_line(exe_path: &Path, game_args: &[String]) -> String {
    let mut l_cmdline = quote_windows_arg(&exe_path.to_string_lossy());

    for l_arg in game_args {
        l_cmdline.push(' ');
        l_cmdline.push_str(&quote_windows_arg(l_arg));
    }

    l_cmdline
}

fn quote_windows_arg(arg: &str) -> String {
    if arg.is_empty()
        || arg
            .chars()
            .any(|l_ch| l_ch == ' ' || l_ch == '\t' || l_ch == '"')
    {
        let mut l_out = String::from("\"");
        let mut l_backslashes = 0usize;

        for l_ch in arg.chars() {
            match l_ch {
                '\\' => {
                    l_backslashes += 1;
                }
                '"' => {
                    l_out.push_str(&"\\".repeat((l_backslashes * 2) + 1));
                    l_out.push('"');
                    l_backslashes = 0;
                }
                _ => {
                    if l_backslashes != 0 {
                        l_out.push_str(&"\\".repeat(l_backslashes));
                        l_backslashes = 0;
                    }
                    l_out.push(l_ch);
                }
            }
        }

        if l_backslashes != 0 {
            l_out.push_str(&"\\".repeat(l_backslashes * 2));
        }

        l_out.push('"');
        l_out
    } else {
        arg.to_string()
    }
}

fn inject_dll(
    pi: &mut PROCESS_INFORMATION,
    wrapper_path: &Path,
    timeout_ms: Option<usize>,
) -> Result<()> {
    let l_wrapper_wide = to_wide_null(wrapper_path.as_os_str());
    let l_bytes = l_wrapper_wide.len() * size_of::<u16>();

    let l_remote_mem = unsafe {
        VirtualAllocEx(
            pi.hProcess,
            null_mut(),
            l_bytes,
            MEM_COMMIT | MEM_RESERVE,
            PAGE_READWRITE,
        )
    };

    if l_remote_mem.is_null() {
        return Err(AppError::Message(format!(
            "VirtualAllocEx failed: {}",
            last_error_message(None)
        )));
    }

    let l_write_ok = unsafe {
        WriteProcessMemory(
            pi.hProcess,
            l_remote_mem,
            l_wrapper_wide.as_ptr() as *const _,
            l_bytes,
            null_mut(),
        )
    };

    if l_write_ok == 0 {
        unsafe { VirtualFreeEx(pi.hProcess, l_remote_mem, 0, MEM_RELEASE) };
        return Err(AppError::Message(format!(
            "WriteProcessMemory failed: {}",
            last_error_message(None)
        )));
    }

    let l_start_routine: windows_sys::Win32::System::Threading::LPTHREAD_START_ROUTINE =
        Some(unsafe { std::mem::transmute(LoadLibraryW as *const () as usize) });

    let l_thread = unsafe {
        CreateRemoteThread(
            pi.hProcess,
            null_mut(),
            0,
            l_start_routine,
            l_remote_mem,
            0,
            null_mut(),
        )
    };

    if l_thread.is_null() {
        unsafe { VirtualFreeEx(pi.hProcess, l_remote_mem, 0, MEM_RELEASE) };
        return Err(AppError::Message(format!(
            "CreateRemoteThread failed: {}",
            last_error_message(None)
        )));
    }

    let l_wait_ms = match timeout_ms {
        Some(l_timeout) => {
            if l_timeout > u32::MAX as usize {
                unsafe { CloseHandle(l_thread) };
                unsafe { VirtualFreeEx(pi.hProcess, l_remote_mem, 0, MEM_RELEASE) };
                return Err(AppError::Message("inject_timeout is too large".to_string()));
            }
            l_timeout as u32
        }
        None => u32::MAX,
    };

    let l_wait_result = unsafe { WaitForSingleObject(l_thread, l_wait_ms) };
    if l_wait_result == WAIT_TIMEOUT {
        unsafe { CloseHandle(l_thread) };
        unsafe { VirtualFreeEx(pi.hProcess, l_remote_mem, 0, MEM_RELEASE) };
        return Err(AppError::Message("wrapper injection timed out".to_string()));
    }

    if l_wait_result != WAIT_OBJECT_0 {
        unsafe { CloseHandle(l_thread) };
        unsafe { VirtualFreeEx(pi.hProcess, l_remote_mem, 0, MEM_RELEASE) };
        return Err(AppError::Message(format!(
            "WaitForSingleObject failed: {}",
            last_error_message(None)
        )));
    }

    let mut l_exit_code: u32 = 0;
    if unsafe { GetExitCodeThread(l_thread, &mut l_exit_code) } == 0 {
        unsafe { CloseHandle(l_thread) };
        unsafe { VirtualFreeEx(pi.hProcess, l_remote_mem, 0, MEM_RELEASE) };
        return Err(AppError::Message(format!(
            "GetExitCodeThread failed: {}",
            last_error_message(None)
        )));
    }

    unsafe { CloseHandle(l_thread) };
    unsafe { VirtualFreeEx(pi.hProcess, l_remote_mem, 0, MEM_RELEASE) };

    if l_exit_code == 0 {
        return Err(AppError::Message(
            "LoadLibraryW failed in remote process".to_string(),
        ));
    }

    Ok(())
}

fn close_process_handles(pi: &mut PROCESS_INFORMATION) {
    if !pi.hThread.is_null() {
        unsafe { CloseHandle(pi.hThread) };
        pi.hThread = std::ptr::null_mut();
    }

    if !pi.hProcess.is_null() {
        unsafe { CloseHandle(pi.hProcess) };
        pi.hProcess = std::ptr::null_mut();
    }
}

fn to_wide_null(value: &OsStr) -> Vec<u16> {
    value.encode_wide().chain(std::iter::once(0)).collect()
}

fn last_error_message(error: Option<u32>) -> String {
    let l_error = error.unwrap_or_else(|| unsafe { GetLastError() });

    let mut l_buffer: *mut u8 = null_mut();

    let l_size = unsafe {
        FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM
                | FORMAT_MESSAGE_IGNORE_INSERTS
                | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            null_mut(),
            l_error,
            0,
            (&mut l_buffer) as *mut _ as *mut u8,
            0,
            null_mut(),
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

fn resolve_path(default_name: &str, r#override: Option<&Path>) -> Result<PathBuf> {
    if let Some(l_path) = r#override {
        return Ok(l_path.to_path_buf());
    }

    let binding = std::env::current_exe()
        .map_err(|l_err| AppError::Message(format!("current_exe failed: {l_err}")))?;
    let l_base_dir = binding
        .parent()
        .ok_or_else(|| AppError::Message("launcher has no parent directory".to_string()))?;

    Ok(l_base_dir.join(default_name))
}

fn validate_launcher_inputs(exe_path: &Path, wrapper_path: &Path, args: &Args) -> Result<()> {
    if !exe_path.exists() {
        return Err(AppError::Message(format!(
            "game executable not found: {}",
            exe_path.display()
        )));
    }

    if !args.no_inject && !wrapper_path.exists() {
        return Err(AppError::Message(format!(
            "wrapper dll not found: {}",
            wrapper_path.display()
        )));
    }

    Ok(())
}
