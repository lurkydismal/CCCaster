use crate::{
    args::Args,
    error::{AppError, Result},
    launcher::{DEFAULT_ADDONS_DIR, DEFAULT_EXE_NAME, DEFAULT_WRAPPER_NAME},
};
use std::path::Path;

mod inject;
mod paths;
mod process;
mod win32;

pub fn run(args: &Args) -> Result<()> {
    if args.attach {
        return Err(AppError::Message(
            "attach mode is not implemented in this launcher".to_string(),
        ));
    }

    let l_exe_path = paths::resolve_path(DEFAULT_EXE_NAME, None)?;
    let l_wrapper_path =
        paths::resolve_path(DEFAULT_WRAPPER_NAME, args.wrapper.as_deref().map(Path::new))?;
    let l_addons_path = paths::resolve_path(
        DEFAULT_ADDONS_DIR,
        args.addons_dir.as_deref().map(Path::new),
    )?;

    if args.validate || args.dry_run {
        paths::validate_launcher_inputs(&l_exe_path, &l_wrapper_path, &l_addons_path, args)?;
        if args.validate {
            println!("validation ok");
        } else {
            println!("dry run ok");
        }
        return Ok(());
    }

    if args.no_inject {
        process::launch_without_injection(&l_exe_path, &args.game_args)?;
        return Ok(());
    }

    process::launch_with_injection(&l_exe_path, &l_wrapper_path, &l_addons_path, args)
}
