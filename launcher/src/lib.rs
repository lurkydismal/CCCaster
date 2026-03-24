pub mod error;

mod args;
mod launcher;
mod profile;

#[cfg(target_os = "windows")]
mod shared_memory;

use std::path::PathBuf;

pub use error::{AppError, Result};

use crate::{args::Args, profile::Profile};
use clap::Parser;

fn collect_env() -> std::collections::BTreeMap<String, String> {
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

    if let Some(profile) = &l_args.profile {
        let mut l_profile_path = PathBuf::from("profiles");
        l_profile_path.push(format!("{}.json", profile));

        if !l_profile_path.exists() {
            launcher_eprintln!("Profile not found: {}", l_profile_path.display());
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

            return launcher::run(&l_new_args);
        }

        if l_args.save_profile {
            profile::save_profile(
                l_profile_path,
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
        }
    }

    launcher::run(&l_args)
}

#[macro_export]
macro_rules! launcher_println {
    ($($arg:tt)*) => {{
        print!("[LAUNCHER] ");
        println!($($arg)*);
    }};
}

#[macro_export]
macro_rules! launcher_trace_println {
    ($trace:expr, $($arg:tt)*) => {
        if $trace {
            print!("[TRACE] ");
            $crate::launcher_println!($($arg)*)
        }
    };
}

#[macro_export]
macro_rules! launcher_eprintln {
    ($($arg:tt)*) => {{
        eprint!("[LAUNCHER] ");
        eprintln!($($arg)*);
    }};
}
