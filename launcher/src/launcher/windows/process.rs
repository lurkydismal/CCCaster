use crate::{
    AppError,
    args::Args,
    launcher::windows::{inject, win32},
    shared_memory,
};
use serde::Serialize;
use std::{ffi::OsStr, os::windows::ffi::OsStrExt, path::Path, ptr};
use windows_sys::Win32::{
    Foundation::{CloseHandle, GetLastError},
    System::Threading::{
        CREATE_SUSPENDED, CreateProcessW, PROCESS_INFORMATION, ResumeThread, STARTUPINFOW,
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

    /// Same as dry-run but stricter: checksum files, detect conflicts, ABI mismatches.
    validate: &'a bool,

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
        copy_fields_ref!(v, { play, dry_run, validate, addon, addons_dir, load_order, no_deps, force, disable, verbose, trace, dump_patches, dump_graph, timings, safe_mode, sandbox, no_patches,  })
    }
}

pub fn launch_without_injection(exe_path: &Path, game_args: &[String]) -> Result<(), AppError> {
    let mut l_pi = spawn_process(exe_path, game_args, false)?;
    println!("process started");
    close_process_handles(&mut l_pi);
    Ok(())
}

pub fn launch_with_injection(
    exe_path: &Path,
    wrapper_path: &Path,
    addons_path: &Path,
    args: &Args,
) -> Result<(), AppError> {
    let mut l_pi = spawn_process(exe_path, &args.game_args, true)?;
    println!("process started suspended");

    let new_args = Args {
        addons_dir: Some(addons_path.to_str().unwrap().to_string()),
        ..args.clone()
    };

    let l_data: WrapperData = (&new_args).into();

    let l_json = serde_json::to_string(&l_data)?;

    println!("json: {}", l_json);

    if let Err(l_err) = shared_memory::write_shared_string("Local\\MySharedData", &l_json) {
        eprintln!("{l_err}");
    }

    inject::inject_dll(&mut l_pi, wrapper_path, args.inject_timeout)?;

    println!("dll injected");

    if args.suspend || args.break_on_load {
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
            &mut l_si,
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
    let mut l_cmdline = quote_windows_arg(&exe_path.to_string_lossy());

    for l_arg in game_args {
        l_cmdline.push(' ');
        l_cmdline.push_str(&quote_windows_arg(l_arg));
    }

    l_cmdline
}

pub fn quote_windows_arg(arg: &str) -> String {
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
    value.encode_wide().chain(std::iter::once(0)).collect()
}
