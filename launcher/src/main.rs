use clap::Parser;
use serde::Serialize;
use std::{
    ffi::{CString, OsStr},
    path::{Path, PathBuf},
    process::ExitCode,
    ptr::copy_nonoverlapping,
};
use windows_sys::Win32::{
    Foundation::HANDLE,
    System::Memory::{CreateFileMappingA, FILE_MAP_WRITE, MapViewOfFile},
};

#[cfg(target_os = "windows")]
use std::{mem::size_of, os::windows::ffi::OsStrExt, ptr::null_mut};

#[cfg(target_os = "windows")]
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

const DEFAULT_EXE_NAME: &str = "MBAA.exe";
const DEFAULT_WRAPPER_NAME: &str = "wrapper.dll";

fn main() -> ExitCode {
    let l_args = Args::parse();

    match run(&l_args) {
        Ok(()) => ExitCode::SUCCESS,
        Err(l_err) => {
            eprintln!("{l_err}");
            ExitCode::from(1)
        }
    }
}

#[cfg(target_os = "windows")]
fn run(args: &Args) -> Result<(), String> {
    if args.attach {
        return Err("attach mode is not implemented in this launcher".to_string());
    }

    let l_exe_path = resolve_path(DEFAULT_EXE_NAME, None)?;
    let l_wrapper_path =
        resolve_path(DEFAULT_WRAPPER_NAME, args.wrapper.as_deref().map(Path::new))?;

    if args.validate || args.dry_run {
        validate_launcher_inputs(&l_exe_path, &l_wrapper_path, args)?;
        if args.validate {
            println!("validation ok");
        } else {
            println!("dry run ok");
        }
        return Ok(());
    }

    if args.no_inject {
        launch_without_injection(&l_exe_path, &args.game_args)?;
        return Ok(());
    }

    launch_with_injection(&l_exe_path, &l_wrapper_path, args)
}

#[cfg(target_os = "windows")]
fn launch_without_injection(exe_path: &Path, game_args: &[String]) -> Result<(), String> {
    let mut l_pi = spawn_process(exe_path, game_args, false)?;
    println!("process started");

    close_process_handles(&mut l_pi);

    Ok(())
}

#[derive(Serialize)]
struct TestData {
    name: String,
    path: String,
}

#[cfg(target_os = "windows")]
fn launch_with_injection(exe_path: &Path, wrapper_path: &Path, args: &Args) -> Result<(), String> {
    let mut l_pi = spawn_process(exe_path, &args.game_args, true)?;
    println!("process started suspended");

    let data = TestData {
        name: "example".to_string(),
        path: "/tmp/file".to_string(),
    };

    let json = match serde_json::to_string(&data) {
        Ok(it) => it,
        Err(err) => return Err(err.to_string()),
    };

    if let Err(e) = write_shared_string("Local\\MySharedData", &json) {
        eprintln!("{}", e);
    }

    inject_dll(&mut l_pi, wrapper_path, args.inject_timeout)?;

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
        return Err(format!(
            "ResumeThread failed: {}",
            last_error_message(Some(l_err))
        ));
    }

    close_process_handles(&mut l_pi);

    Ok(())
}

#[cfg(target_os = "windows")]
fn spawn_process(
    exe_path: &Path,
    game_args: &[String],
    suspended: bool,
) -> Result<PROCESS_INFORMATION, String> {
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
        return Err(format!(
            "CreateProcessW failed: {}",
            last_error_message(None)
        ));
    }

    Ok(l_pi)
}

#[cfg(target_os = "windows")]
fn build_command_line(exe_path: &Path, game_args: &[String]) -> String {
    let mut l_cmdline = quote_windows_arg(&exe_path.to_string_lossy());

    for l_arg in game_args {
        l_cmdline.push(' ');
        l_cmdline.push_str(&quote_windows_arg(l_arg));
    }

    l_cmdline
}

#[cfg(target_os = "windows")]
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

#[cfg(target_os = "windows")]
fn inject_dll(
    pi: &mut PROCESS_INFORMATION,
    wrapper_path: &Path,
    timeout_ms: Option<usize>,
) -> Result<(), String> {
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
        return Err(format!(
            "VirtualAllocEx failed: {}",
            last_error_message(None)
        ));
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
        return Err(format!(
            "WriteProcessMemory failed: {}",
            last_error_message(None)
        ));
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
        return Err(format!(
            "CreateRemoteThread failed: {}",
            last_error_message(None)
        ));
    }

    let l_wait_ms = match timeout_ms {
        Some(l_timeout) => {
            if l_timeout > u32::MAX as usize {
                unsafe { CloseHandle(l_thread) };
                unsafe { VirtualFreeEx(pi.hProcess, l_remote_mem, 0, MEM_RELEASE) };
                return Err("inject_timeout is too large".to_string());
            }
            l_timeout as u32
        }
        None => u32::MAX,
    };

    let l_wait_result = unsafe { WaitForSingleObject(l_thread, l_wait_ms) };
    if l_wait_result == WAIT_TIMEOUT {
        unsafe { CloseHandle(l_thread) };
        unsafe { VirtualFreeEx(pi.hProcess, l_remote_mem, 0, MEM_RELEASE) };
        return Err("wrapper injection timed out".to_string());
    }

    if l_wait_result != WAIT_OBJECT_0 {
        unsafe { CloseHandle(l_thread) };
        unsafe { VirtualFreeEx(pi.hProcess, l_remote_mem, 0, MEM_RELEASE) };
        return Err(format!(
            "WaitForSingleObject failed: {}",
            last_error_message(None)
        ));
    }

    let mut l_exit_code: u32 = 0;
    if unsafe { GetExitCodeThread(l_thread, &mut l_exit_code) } == 0 {
        unsafe { CloseHandle(l_thread) };
        unsafe { VirtualFreeEx(pi.hProcess, l_remote_mem, 0, MEM_RELEASE) };
        return Err(format!(
            "GetExitCodeThread failed: {}",
            last_error_message(None)
        ));
    }

    unsafe { CloseHandle(l_thread) };
    unsafe { VirtualFreeEx(pi.hProcess, l_remote_mem, 0, MEM_RELEASE) };

    if l_exit_code == 0 {
        return Err("LoadLibraryW failed in remote process".to_string());
    }

    Ok(())
}

#[cfg(target_os = "windows")]
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

#[cfg(target_os = "windows")]
fn to_wide_null(value: &OsStr) -> Vec<u16> {
    value.encode_wide().chain(std::iter::once(0)).collect()
}

#[cfg(target_os = "windows")]
pub fn last_error_message(error: Option<u32>) -> String {
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

    // trim trailing \r\n
    while l_string.ends_with(['\r', '\n']) {
        l_string.pop();
    }

    unsafe { LocalFree(l_buffer as HLOCAL) };

    format!("{} ({})", l_string, l_error)
}

#[repr(C)]
struct MyData {
    size: usize,
    value: [u8; 0],
}

fn write_shared_string(name: &str, value: &str) -> Result<(), &'static str> {
    let size = std::mem::size_of::<MyData>() + value.len();
    let c_name = CString::new(name).map_err(|_| "invalid mapping name")?;

    let mapping: HANDLE = unsafe {
        CreateFileMappingA(
            -1isize as HANDLE, // INVALID_HANDLE_VALUE
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
        return Err("MapViewOfFile failed");
    }

    // Write header
    let header = view.Value as *mut MyData;
    unsafe {
        (*header).size = value.len();
    }

    // Write payload right after header
    let data_ptr = (unsafe { (*header).value.as_ptr() }) as *mut u8;

    unsafe { copy_nonoverlapping(value.as_ptr(), data_ptr, value.len()) };

    Ok(())
}

#[cfg(target_os = "linux")]
fn run(_args: &Args) -> Result<(), String> {
    unimplemented!("this launcher implementation is Windows-only right now")
}

fn resolve_path(default_name: &str, r#override: Option<&Path>) -> Result<PathBuf, String> {
    if let Some(l_path) = r#override {
        return Ok(l_path.to_path_buf());
    }

    let binding =
        std::env::current_exe().map_err(|l_err| format!("current_exe failed: {l_err}"))?;
    let l_base_dir = binding
        .parent()
        .ok_or_else(|| "launcher has no parent directory".to_string())?;

    Ok(l_base_dir.join(default_name))
}

fn validate_launcher_inputs(
    exe_path: &Path,
    wrapper_path: &Path,
    args: &Args,
) -> Result<(), String> {
    if !exe_path.exists() {
        return Err(format!("game executable not found: {}", exe_path.display()));
    }

    if !args.no_inject && !wrapper_path.exists() {
        return Err(format!("wrapper dll not found: {}", wrapper_path.display()));
    }

    Ok(())
}

#[derive(Parser, Debug)]
#[command(
    version,
    about,
    long_about = color_print::cstr!(r#"<bold><underline>Launcher Overview</underline></bold>

This tool controls how the game is started with addon support enabled. It handles discovery, validation, dependency resolution, and optional patching before launching the process.

<bold>Execution flow:</bold>
<dim>1.</dim> Parse arguments  
<dim>2.</dim> Discover addons  
<dim>3.</dim> Validate dependencies and compatibility  
<dim>4.</dim> Resolve load order  
<dim>5.</dim> Apply patches and prepare runtime  
<dim>6.</dim> Launch the game process  
<dim>7.</dim> Inject wrapper and transfer control  

<bold>Key capabilities:</bold>
<dim>•</dim> Selective addon loading and profiles  
<dim>•</dim> Strict or relaxed validation modes  
<dim>•</dim> Full control over load order and dependencies  
<dim>•</dim> Injection and runtime hooking via wrapper  
<dim>•</dim> Debugging and tracing tools  
<dim>•</dim> Safe, dry-run, and validation-only execution  

<bold>Usage examples:</bold>

<dim>$</dim> <bold>launcher --profile modded</bold>  
<dim>$</dim> <bold>launcher --validate</bold>  
<dim>$</dim> <bold>launcher --no-inject -- game.exe -fullscreen</bold>

<bold>Notes:</bold>
<dim>•</dim> Use <bold>--</bold> to pass arguments directly to the game  
<dim>•</dim> Modes like <bold>--dry-run</bold> do not start the game  
<dim>•</dim> Injection behavior can be controlled or disabled  
"#)
)]
struct Args {
    // Core execution modes
    /// Default behavior. Launch + inject + run normally.
    #[arg(long)]
    play: bool,

    /// Launch game without touching it (baseline comparison, debugging crashes).
    #[arg(long)]
    no_inject: bool,

    /// Validate addons, dependency graph, patches, paths — do not launch.
    #[arg(long)]
    dry_run: bool,

    /// Same as dry-run but stricter: checksum files, detect conflicts, ABI mismatches.
    #[arg(long)]
    validate: bool,

    /// Load only specific addons (override auto-load).
    #[arg(short, long, value_name = "NAME", action = clap::ArgAction::Append)]
    addon: Vec<String>,

    /// Load a predefined addon set.
    #[arg(short, long, value_name = "NAME")]
    profile: Option<String>,

    // Addon loading & resolution control
    /// Override default `addons/`.
    #[arg(long, value_name = "PATH")]
    addons_dir: Option<String>,

    /// Explicit load order override (bypass dependency resolver).
    #[arg(long, value_name = "FILE")]
    load_order: Option<String>,

    /// Ignore dependencies (dangerous but useful for debugging).
    #[arg(long)]
    no_deps: bool,

    /// Ignore version/ API mismatches.
    #[arg(short, long)]
    force: bool,

    /// Blacklist specific addons.
    #[arg(short, long, value_name = "ADDON", action = clap::ArgAction::Append)]
    disable: Vec<String>,

    // Injection/ runtime control
    /// Custom wrapper DLL.
    #[arg(short, long, value_name = "PATH")]
    wrapper: Option<String>,

    /// Fail if wrapper doesn’t signal readiness.
    #[arg(short, long, value_name = "MILLISECONDS")]
    inject_timeout: Option<usize>,

    /// Resume immediately after injection (useful if wrapper is optional).
    #[arg(long)]
    no_wait_wrapper: bool,

    /// Keep process suspended after injection (for manual debugging with external tools).
    #[arg(long)]
    suspend: bool,

    /// Attach to an already running process instead of spawning.
    #[arg(long)]
    attach: bool,

    // Debugging & diagnostics
    /// Increase logging verbosity (-v, -vv, -vvv).
    /// Each additional `-v` increases detail level.
    #[arg(short, long, action = clap::ArgAction::Count)]
    verbose: u8,

    /// Write logs to the specified file instead of stdout.
    #[arg(short, long, value_name = "PATH")]
    log_file: Option<String>,

    /// Very noisy: patching, hooks, loader internals.
    #[arg(long)]
    trace: bool,

    /// Output resolved patches after dependency resolution.
    #[arg(long)]
    dump_patches: bool,

    /// Output mod dependency graph.
    #[arg(long)]
    dump_graph: bool,

    /// Measure load/ injection phases.
    #[arg(long)]
    timings: bool,

    /// Break before resuming process (for debugger).
    #[arg(short, long)]
    break_on_load: bool,

    // Safety/ isolation controls
    /// Disable all mods except core/ runtime.
    #[arg(long)]
    safe_mode: bool,

    /// Restrict file access (NOTE: stub, no VFS yet).
    #[arg(short, long)]
    sandbox: bool,

    /// Load mods but don’t apply binary patches (script-only testing).
    #[arg(long)]
    no_patches: bool,

    // Game argument passthrough
    /// Everything after goes directly to the game executable.
    #[arg(trailing_var_arg = true, allow_hyphen_values = true)]
    game_args: Vec<String>,
}
