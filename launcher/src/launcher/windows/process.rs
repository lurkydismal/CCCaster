use crate::{
    AppError,
    args::Args,
    launcher::windows::{inject, win32},
    launcher_debug, launcher_info, launcher_trace, launcher_warning, shared_memory,
};
use serde::Serialize;
use std::{ffi::OsStr, os::windows::ffi::OsStrExt, path::Path, ptr};
use windows_sys::Win32::{
    Foundation::{CloseHandle, GetLastError, HANDLE, INVALID_HANDLE_VALUE},
    System::{
        Diagnostics::{
            Debug::DebugBreak,
            ToolHelp::{
                CreateToolhelp32Snapshot, PROCESSENTRY32, Process32First, Process32Next,
                TH32CS_SNAPPROCESS,
            },
        },
        Threading::{
            CREATE_SUSPENDED, CreateProcessW, GetCurrentProcessId, OpenProcess, PROCESS_ALL_ACCESS,
            PROCESS_INFORMATION, ResumeThread, STARTUPINFOW,
        },
    },
};

macro_rules! copy_fields_ref {
    ($src:expr, { $($f:ident),* $(,)? }) => {
        Self {
            $($f: &$src.$f),*
        }
    };
}

#[derive(Serialize)]
struct WrapperData<'a> {
    // Core execution modes
    /// Default behavior. Launch + inject + run normally.
    play: &'a bool,

    /// Validate addons, dependency graph, patches, paths — do not launch.
    dry_run: &'a bool,

    /// Load only specific addons (override auto-load).
    addon: &'a Option<Vec<String>>,

    // Addon loading & resolution control
    /// Override default `addons/`.
    addons_dir: &'a Option<String>,

    /// Explicit load order override (bypass dependency resolver).
    load_order: &'a Option<String>,

    /// Ignore dependencies (dangerous but useful for debugging).
    no_deps: &'a bool,

    /// Ignore version/ API mismatches.
    force: &'a bool,

    /// Blacklist specific addons.
    disable: &'a Option<Vec<String>>,

    // Debugging & diagnostics
    /// Increase logging verbosity (-v, -vv, -vvv).
    /// Each additional `-v` increases detail level.
    verbose: &'a u8,

    /// Very noisy: patching, hooks, loader internals.
    trace: &'a bool,

    /// Output resolved patches after dependency resolution.
    dump_patches: &'a bool,

    /// Output mod dependency graph.
    dump_graph: &'a bool,

    /// Measure load/ injection phases.
    timings: &'a bool,

    // Safety/ isolation controls
    /// Disable all mods except core/ runtime.
    safe_mode: &'a bool,

    /// Restrict file access (NOTE: stub, no VFS yet).
    sandbox: &'a bool,

    /// Load mods but don’t apply binary patches (script-only testing).
    no_patches: &'a bool,
}

impl<'a> From<&'a Args> for WrapperData<'a> {
    fn from(v: &'a Args) -> Self {
        launcher_trace!(
            "WrapperData::from accepted args: trace={}, verbose={}, dry_run={}, addon_count={}",
            v.trace,
            v.verbose,
            v.dry_run,
            v.addon.as_ref().map_or(0, |l_v| l_v.len())
        );
        copy_fields_ref!(v, { play, dry_run, addon, addons_dir, load_order, no_deps, force, disable, verbose, trace, dump_patches, dump_graph, timings, safe_mode, sandbox, no_patches, })
    }
}

pub fn launch_without_injection(exe_path: &Path, game_args: &[String]) -> Result<(), AppError> {
    launcher_trace!(
        "launch_without_injection accepted args: exe_path={}, game_args_count={}",
        exe_path.display(),
        game_args.len()
    );
    let mut l_pi = spawn_process(exe_path, game_args, false)?;
    launcher_info!("process started");
    close_process_handles(&mut l_pi);
    Ok(())
}

#[derive(Debug)]
pub struct ProcessHandle {
    #[expect(unused)]
    pub pid: u32,
    pub handle: HANDLE,
}

pub fn attach_to_process_by_name(process_name: &str) -> Option<ProcessHandle> {
    launcher_trace!("attach_to_process_by_name accepted args: process_name={process_name}");
    let l_snapshot = unsafe { CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) };
    if l_snapshot == INVALID_HANDLE_VALUE {
        return None;
    }

    let l_target = std::ffi::CString::new(process_name).ok()?;
    let mut l_entry: PROCESSENTRY32 = unsafe { std::mem::zeroed() };
    l_entry.dwSize = std::mem::size_of::<PROCESSENTRY32>() as u32;

    if unsafe { Process32First(l_snapshot, &mut l_entry) } == 0 {
        unsafe { CloseHandle(l_snapshot) };
        return None;
    }

    loop {
        let l_exe_name = {
            let l_len = l_entry
                .szExeFile
                .iter()
                .position(|&c| c == 0)
                .unwrap_or(260);

            let l_bytes: &[u8] = unsafe {
                std::slice::from_raw_parts(l_entry.szExeFile.as_ptr() as *const u8, l_len)
            };

            std::ffi::CString::new(l_bytes).unwrap()
        };

        if l_exe_name.as_c_str() == l_target.as_c_str() {
            let l_pid = l_entry.th32ProcessID;

            let l_handle = unsafe { OpenProcess(PROCESS_ALL_ACCESS, 0, l_pid) };
            unsafe { CloseHandle(l_snapshot) };

            if l_handle.is_null() {
                return None;
            }

            return Some(ProcessHandle {
                pid: l_pid,
                handle: l_handle,
            });
        }

        if unsafe { Process32Next(l_snapshot, &mut l_entry) } == 0 {
            break;
        }
    }

    unsafe { CloseHandle(l_snapshot) };
    None
}

pub fn current_process_handle() -> Option<HANDLE> {
    launcher_trace!("current_process_handle accepted args: none");
    let l_pid = unsafe { GetCurrentProcessId() };
    let l_h_process = unsafe { OpenProcess(PROCESS_ALL_ACCESS, 0, l_pid) };

    if l_h_process.is_null() {
        None
    } else {
        Some(l_h_process)
    }
}

pub fn inject_into_running_process(
    h_process: HANDLE,
    wrapper_path: &Path,
    addons_path: &Path,
    args: &Args,
) -> Result<(), AppError> {
    launcher_trace!(
        "inject_into_running_process accepted args: h_process={h_process:p}, wrapper_path={}, addons_path={}, trace={}, verbose={}",
        wrapper_path.display(),
        addons_path.display(),
        args.trace,
        args.verbose
    );
    let l_new_args = Args {
        addons_dir: Some(addons_path.to_str().unwrap().to_string()),
        ..args.clone()
    };

    let l_data: WrapperData = (&l_new_args).into();
    let l_json = serde_json::to_string(&l_data)?;

    launcher_trace!("json: {}", l_json);

    if let Err(l_err) = shared_memory::write_shared_string("Local\\MySharedData", &l_json) {
        launcher_warning!("{l_err}");
    }

    inject::inject_dll(
        h_process,
        wrapper_path,
        args.inject_timeout,
        !args.no_wait_wrapper,
    )?;

    launcher_info!("dll injected");
    Ok(())
}

pub fn launch_with_injection(
    exe_path: &Path,
    wrapper_path: &Path,
    addons_path: &Path,
    args: &Args,
) -> Result<(), AppError> {
    launcher_trace!(
        "launch_with_injection accepted args: exe_path={}, wrapper_path={}, addons_path={}, game_args_count={}, suspend={}",
        exe_path.display(),
        wrapper_path.display(),
        addons_path.display(),
        args.game_args.len(),
        args.suspend
    );
    let mut l_pi = spawn_process(exe_path, &args.game_args, true)?;
    launcher_info!("process started suspended");

    inject_into_running_process(l_pi.hProcess, wrapper_path, addons_path, args)?;

    if args.break_on_load {
        launcher_debug!("break before resuming process");
        unsafe {
            DebugBreak();
        }
    }

    if args.suspend {
        launcher_info!("leaving process suspended");
        close_process_handles(&mut l_pi);
        return Ok(());
    }

    launcher_info!("resuming process");
    let l_resume_result = unsafe { ResumeThread(l_pi.hThread) };
    if l_resume_result == u32::MAX {
        let l_err = unsafe { GetLastError() };
        close_process_handles(&mut l_pi);
        return Err(AppError::Message(format!(
            "ResumeThread failed: {}",
            win32::last_error_message(Some(l_err))
        )));
    }

    close_process_handles(&mut l_pi);
    Ok(())
}

pub fn spawn_process(
    exe_path: &Path,
    game_args: &[String],
    suspended: bool,
) -> Result<PROCESS_INFORMATION, AppError> {
    launcher_trace!(
        "spawn_process accepted args: exe_path={}, game_args_count={}, suspended={}",
        exe_path.display(),
        game_args.len(),
        suspended
    );
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
            ptr::null_mut(),
            ptr::null_mut(),
            0,
            l_flags,
            ptr::null_mut(),
            ptr::null_mut(),
            &l_si,
            &mut l_pi,
        )
    };

    if l_ok == 0 {
        return Err(AppError::Message(format!(
            "CreateProcessW failed: {}",
            win32::last_error_message(None)
        )));
    }

    Ok(l_pi)
}

pub fn build_command_line(exe_path: &Path, game_args: &[String]) -> String {
    launcher_trace!(
        "build_command_line accepted args: exe_path={}, game_args_count={}",
        exe_path.display(),
        game_args.len()
    );
    let mut l_cmdline = quote_windows_arg(&exe_path.to_string_lossy());

    for l_arg in game_args {
        l_cmdline.push(' ');
        l_cmdline.push_str(&quote_windows_arg(l_arg));
    }

    l_cmdline
}

pub fn quote_windows_arg(arg: &str) -> String {
    launcher_trace!("quote_windows_arg accepted args: arg_len={}", arg.len());
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

pub fn close_process_handles(pi: &mut PROCESS_INFORMATION) {
    launcher_trace!(
        "close_process_handles accepted args: hThread={:p}, hProcess={:p}",
        pi.hThread,
        pi.hProcess
    );
    if !pi.hThread.is_null() {
        unsafe { CloseHandle(pi.hThread) };
        pi.hThread = std::ptr::null_mut();
    }

    if !pi.hProcess.is_null() {
        unsafe { CloseHandle(pi.hProcess) };
        pi.hProcess = std::ptr::null_mut();
    }
}

pub fn to_wide_null(value: &OsStr) -> Vec<u16> {
    launcher_trace!("to_wide_null accepted args");
    value.encode_wide().chain(std::iter::once(0)).collect()
}
