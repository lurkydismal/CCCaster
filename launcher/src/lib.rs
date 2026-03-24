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
