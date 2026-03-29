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

pub fn set_enabled_log_level(level: u8) {
    modloader_debug!("set_enabled_log_level called with level={}", level);
    ENABLED_LOG_LEVEL.store(level, Ordering::Relaxed);
}

pub fn enabled_log_level() -> u8 {
    ENABLED_LOG_LEVEL.load(Ordering::Relaxed)
}

pub fn parse_args(json: &str) -> Result<()> {
    modloader_debug!("parse_args received json payload ({} bytes)", json.len());
    match serde_json::from_str(json) {
        Ok(data) => {
            set_enabled_log_level(log_level_from_args(&data));

            // TODO: Get env vars and apply

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
            modloader_error!("{err}");

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
    loader::load_mods_from_addons().await?;
    Ok(())
}
