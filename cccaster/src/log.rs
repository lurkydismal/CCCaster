pub mod error;

mod MODLOADER;
mod args;
mod profile;

#[cfg(target_os = "windows")]
mod shared_memory;

use std::path::PathBuf;
use std::sync::atomic::{AtomicU8, Ordering};

pub use error::{AppError, Result};

use crate::{args::Args, profile::Profile};
use clap::Parser;

pub const LOG_ERROR: u8 = 0;
pub const LOG_WARNING: u8 = 1;
pub const LOG_INFO: u8 = 2;
pub const LOG_DEBUG: u8 = 3;
pub const LOG_TRACE: u8 = 4;

static ENABLED_LOG_LEVEL: AtomicU8 = AtomicU8::new(LOG_ERROR);

pub fn log_level_from_args(args: &Args) -> u8 {
    modloader_trace!(
        "log_level_from_args accepted args: trace={}, verbose={}",
        args.trace,
        args.verbose
    );
    if args.trace {
        LOG_TRACE
    } else {
        args.verbose.min(LOG_DEBUG)
    }
}

pub fn set_enabled_log_level(level: u8) {
    ENABLED_LOG_LEVEL.store(level, Ordering::Relaxed);
}

pub fn enabled_log_level() -> u8 {
    ENABLED_LOG_LEVEL.load(Ordering::Relaxed)
}

fn collect_env() -> std::collections::BTreeMap<String, String> {
    modloader_trace!("collect_env accepted args");
    let mut l_map = std::collections::BTreeMap::new();

    for (l_key, l_value) in std::env::vars() {
        if l_key.starts_with("MODLOADER_") {
            l_map.insert(l_key, l_value);
        }
    }

    l_map
}

pub fn parse_args() -> Result<()> {
    let l_args = args::Args::parse();
    set_enabled_log_level(log_level_from_args(&l_args));
    modloader_trace!(
        "parse_args accepted args: addons_dir={:?}, load_order={:?}, trace={}, verbose={}",
        l_args.addons_dir,
        l_args.load_order,
        l_args.trace,
        l_args.verbose,
    );
}

#[macro_export]
macro_rules! modloader_trace {
    ($($arg:tt)*) => {{
        if $crate::enabled_log_level() >= $crate::LOG_TRACE {
            color_print::ceprint!("<m!>[MODLOADER]</> [TRACE] ");
            eprintln!($($arg)*);
        }
    }};
}

#[macro_export]
macro_rules! modloader_debug {
    ($($arg:tt)*) => {{
        if $crate::enabled_log_level() >= $crate::LOG_DEBUG {
            color_print::cprint!("<m!>[MODLOADER]</> <c!>[DEBUG]</> ");
            println!($($arg)*);
        }
    }};
}

#[macro_export]
macro_rules! modloader_info {
    ($($arg:tt)*) => {{
        if $crate::enabled_log_level() >= $crate::LOG_INFO {
            color_print::cprint!("<m!>[MODLOADER]</> <g!>[INFO]</> ");
            println!($($arg)*);
        }
    }};
}

#[macro_export]
macro_rules! modloader_warning {
    ($($arg:tt)*) => {{
        if $crate::enabled_log_level() >= $crate::LOG_WARNING {
            color_print::ceprint!("<m!>[MODLOADER]</> <y!>[WARNING]</> ");
            eprintln!($($arg)*);
        }
    }};
}

#[macro_export]
macro_rules! modloader_error {
    ($($arg:tt)*) => {{
        if $crate::enabled_log_level() >= $crate::LOG_ERROR {
            color_print::ceprint!("<m!>[MODLOADER]</> <r!>[ERROR]</> ");
            eprintln!($($arg)*);
        }
    }};
}
