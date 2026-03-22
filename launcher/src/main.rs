mod args;
mod launcher;
mod shared_memory;

use clap::Parser;
use std::process::ExitCode;

fn main() -> ExitCode {
    let l_args = args::Args::parse();

    match launcher::run(&l_args) {
        Ok(()) => ExitCode::SUCCESS,
        Err(l_err) => {
            eprintln!("{l_err}");
            ExitCode::from(1)
        }
    }
}
