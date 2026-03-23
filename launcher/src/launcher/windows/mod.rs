use crate::{
    args::Args,
    error::{AppError, Result},
    launcher::{DEFAULT_EXE_NAME, DEFAULT_WRAPPER_NAME},
};
use std::path::Path;

mod inject;
mod paths;
mod process;
mod win32;

pub fn run(l_args: &Args) -> Result<()> {
    if l_args.attach {
        return Err(AppError::Message(
            "attach mode is not implemented in this launcher".to_string(),
        ));
    }

    let l_exe_path = paths::resolve_path(DEFAULT_EXE_NAME, None)?;
    let l_wrapper_path = paths::resolve_path(
        DEFAULT_WRAPPER_NAME,
        l_args.wrapper.as_deref().map(Path::new),
    )?;

    if l_args.validate || l_args.dry_run {
        paths::validate_launcher_inputs(&l_exe_path, &l_wrapper_path, l_args)?;
        if l_args.validate {
            println!("validation ok");
        } else {
            println!("dry run ok");
        }
        return Ok(());
    }

    if l_args.no_inject {
        process::launch_without_injection(&l_exe_path, &l_args.game_args)?;
        return Ok(());
    }

    process::launch_with_injection(&l_exe_path, &l_wrapper_path, l_args)
}
