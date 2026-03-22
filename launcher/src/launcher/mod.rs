pub const DEFAULT_EXE_NAME: &str = "MBAA.exe";
pub const DEFAULT_WRAPPER_NAME: &str = "wrapper.dll";

#[cfg(target_os = "windows")]
mod windows;

#[cfg(target_os = "linux")]
mod linux;

#[cfg(target_os = "windows")]
pub use windows::run;

#[cfg(target_os = "linux")]
pub use linux::run;
