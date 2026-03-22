pub mod error;

mod args;
mod launcher;

#[cfg(target_os = "windows")]
mod shared_memory;

pub use error::{AppError, Result};

use clap::Parser;

pub fn run_cli() -> Result<()> {
    let l_args = args::Args::parse();
    launcher::run(&l_args)
}
