pub mod error;

mod args;
mod launcher;
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
    launcher_trace!(
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
    launcher_trace!("collect_env accepted args");
    let mut l_map = std::collections::BTreeMap::new();

    for (l_key, l_value) in std::env::vars() {
        if l_key.starts_with("LAUNCHER_")
            || l_key.starts_with("MODLOADER_")
            || l_key.starts_with("WRAPPER_")
        {
            l_map.insert(l_key, l_value);
        }
    }

    l_map
}

pub fn run_cli() -> Result<()> {
    let l_args = args::Args::parse();
    set_enabled_log_level(log_level_from_args(&l_args));
    launcher_trace!(
        "run_cli accepted args: profile={:?}, save_profile={}, addons_dir={:?}, load_order={:?}, trace={}, verbose={}, game_args_count={}",
        l_args.profile,
        l_args.save_profile,
        l_args.addons_dir,
        l_args.load_order,
        l_args.trace,
        l_args.verbose,
        l_args.game_args.len()
    );

    let l_root = if let Ok(l_root) = std::env::var("LAUNCHER_ROOT") {
        launcher_debug!("Using LAUNCHER_ROOT override: {l_root}");
        PathBuf::from(l_root)
    } else {
        let l_current_exe = std::env::current_exe().map_err(|l_err| {
            AppError::Message(format!(
                "Failed to resolve current executable path: {l_err}"
            ))
        })?;
        launcher_debug!(
            "Resolved current executable path: {}",
            l_current_exe.display()
        );

        let l_canonical_exe = l_current_exe.canonicalize().map_err(|l_err| {
            AppError::Message(format!(
                "Failed to canonicalize executable path '{}': {l_err}",
                l_current_exe.display()
            ))
        })?;
        launcher_debug!("Canonical executable path: {}", l_canonical_exe.display());

        l_canonical_exe.parent().map(PathBuf::from).ok_or_else(|| {
            AppError::Message(format!(
                "Executable path has no parent directory: {}",
                l_canonical_exe.display()
            ))
        })?
    };
    launcher_debug!("Resolved launcher root: {}", l_root.display());

    let l_env_profile = std::env::var("LAUNCHER_PROFILE").ok();
    let l_profile = l_args.profile.as_deref().or(l_env_profile.as_deref());

    if let Some(profile) = l_profile {
        let l_profile_path = l_root.join("profiles").join(format!("{}.json", profile));

        if l_args.save_profile {
            profile::save_profile(
                &l_profile_path,
                &Profile {
                    enabled_mods: l_args
                        .addon
                        .as_ref()
                        .into_iter()
                        .flatten()
                        .cloned()
                        .collect(),
                    load_order: l_args.load_order.as_deref().map(|l_s| {
                        l_s.split(';')
                            .map(|l_part| l_part.to_string())
                            .collect::<Vec<String>>()
                    }),
                    env: collect_env(),
                    game_args: l_args.game_args.clone(),
                },
            )?;

            launcher_info!("Profile saved to: {}", &l_profile_path.display());
        }

        if !l_profile_path.exists() {
            launcher_error!("Profile not found: {}", l_profile_path.display());
        } else {
            let l_profile = profile::load_profile(l_profile_path)?;

            let mut l_env = std::collections::BTreeMap::new();
            l_profile.set_env_overrides_into(&mut l_env);

            let mut l_game_args = l_args.game_args.clone();
            l_profile.append_game_args_into(&mut l_game_args);

            let l_new_args = Args {
                load_order: Some(l_profile.load_order_string()),
                game_args: l_game_args,
                profile: None,
                ..l_args.clone()
            };

            return launcher::run(&l_new_args, &l_root);
        }
    } else {
        if l_args.save_profile {
            launcher_error!("Profile name not specified");
        }
    }

    launcher::run(&l_args, &l_root)
}

#[macro_export]
macro_rules! launcher_trace {
    ($($arg:tt)*) => {{
        if $crate::enabled_log_level() >= $crate::LOG_TRACE {
            eprint!("[LAUNCHER] [TRACE] ");
            eprintln!($($arg)*);
        }
    }};
}

#[macro_export]
macro_rules! launcher_debug {
    ($($arg:tt)*) => {{
        if $crate::enabled_log_level() >= $crate::LOG_DEBUG {
            print!("[LAUNCHER] [DEBUG] ");
            println!($($arg)*);
        }
    }};
}

#[macro_export]
macro_rules! launcher_info {
    ($($arg:tt)*) => {{
        if $crate::enabled_log_level() >= $crate::LOG_INFO {
            print!("[LAUNCHER] [INFO] ");
            println!($($arg)*);
        }
    }};
}

#[macro_export]
macro_rules! launcher_warning {
    ($($arg:tt)*) => {{
        if $crate::enabled_log_level() >= $crate::LOG_WARNING {
            eprint!("[LAUNCHER] [WARNING] ");
            eprintln!($($arg)*);
        }
    }};
}

#[macro_export]
macro_rules! launcher_error {
    ($($arg:tt)*) => {{
        if $crate::enabled_log_level() >= $crate::LOG_ERROR {
            eprint!("[LAUNCHER] [ERROR] ");
            eprintln!($($arg)*);
        }
    }};
}
