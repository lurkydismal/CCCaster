//! CCCaster modloader entrypoint and shared runtime utilities.
//!
//! This module defines:
//! - coarse-grained log levels,
//! - log macros used by all submodules,
//! - argument parsing with environment overrides, and
//! - the async main entrypoint used by the C API bridge.

pub mod error;

mod api;
mod args;
mod loader;
mod patch;
mod types;

use std::sync::atomic::{AtomicU8, Ordering};

pub use error::{AppError, Result};

use crate::args::ModloaderData;

pub const LOG_ERROR: u8 = 0;
pub const LOG_WARNING: u8 = 1;
pub const LOG_INFO: u8 = 2;
pub const LOG_DEBUG: u8 = 3;
pub const LOG_TRACE: u8 = 4;

static ENABLED_LOG_LEVEL: AtomicU8 = AtomicU8::new(LOG_ERROR);

/// Computes the effective log level from parsed launcher arguments.
pub fn log_level_from_args(args: &ModloaderData) -> u8 {
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

/// Updates the global log-level gate used by logging macros.
pub fn set_enabled_log_level(level: u8) {
    modloader_debug!("set_enabled_log_level called with level={}", level);
    ENABLED_LOG_LEVEL.store(level, Ordering::Relaxed);
}

/// Reads the currently enabled global log level.
pub fn enabled_log_level() -> u8 {
    ENABLED_LOG_LEVEL.load(Ordering::Relaxed)
}

fn env_bool(name: &str) -> Option<bool> {
    modloader_trace!("Reading boolean env override: {}", name);
    std::env::var(name)
        .ok()
        .and_then(|v| match v.to_ascii_lowercase().as_str() {
            "1" | "true" | "yes" | "on" => Some(true),
            "0" | "false" | "no" | "off" => Some(false),
            _ => None,
        })
}

fn env_string(name: &str) -> Option<String> {
    modloader_trace!("Reading string env override: {}", name);
    std::env::var(name).ok()
}

/// Parses the JSON argument payload received from the host process.
///
/// The payload is deserialized into [`ModloaderData`] and then adjusted with
/// optional environment-based developer overrides.
pub fn parse_args(json: &str) -> Result<()> {
    modloader_debug!("parse_args received json payload ({} bytes)", json.len());
    match serde_json::from_str::<ModloaderData>(json) {
        Ok(mut data) => {
            // --- overrides ---
            if env_bool("MODLOADER_DEBUG").unwrap_or(false) {
                modloader_debug!("Applying MODLOADER_DEBUG=true override");
                data.verbose = data.verbose.max(LOG_DEBUG);
            }

            if let Some(v) = env_bool("MODLOADER_TRACE") {
                modloader_debug!("Applying MODLOADER_TRACE override: {}", v);
                data.trace = data.trace || v;
            }

            if let Some(v) = env_string("MODLOADER_LOAD_ORDER") {
                modloader_debug!("Applying MODLOADER_LOAD_ORDER override: {}", v);
                data.load_order = Some(v);
            }
            // ------------------

            set_enabled_log_level(log_level_from_args(&data));

            modloader_trace!(
                "parse_args accepted args: play={}, dry_run={}, addon={:?}, addons_dir={:?}, load_order={:?}, no_deps={}, force={}, disable={:?}, trace={}, verbose={}, dump_patches={}, dump_graph={}, timings={}, safe_mode={}, sandbox={}",
                data.play,
                data.dry_run,
                data.addon,
                data.addons_dir,
                data.load_order,
                data.no_deps,
                data.force,
                data.disable,
                data.trace,
                data.verbose,
                data.dump_patches,
                data.dump_graph,
                data.timings,
                data.safe_mode,
                data.sandbox,
            );

            modloader_info!("CLI/runtime arguments parsed successfully");

            Ok(())
        }
        Err(err) => {
            modloader_error!("Failed to deserialize ModloaderData from init JSON: {err}");

            Err(AppError::Message(err.to_string()))
        }
    }
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

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    loader::load_mods_from_addons().await
}
