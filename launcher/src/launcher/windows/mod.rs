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
    let l_exe_path = paths::resolve_path(DEFAULT_EXE_NAME, None)?;
    let l_wrapper_path =
        paths::resolve_path(DEFAULT_WRAPPER_NAME, args.wrapper.as_deref().map(Path::new))?;
    let l_addons_path = paths::resolve_path(
        DEFAULT_ADDONS_DIR,
        args.addons_dir.as_deref().map(Path::new),
    )?;

    if !args.play {
        if args.dry_run {
            paths::validate_launcher_inputs(&l_exe_path, &l_wrapper_path, &l_addons_path, args)?;

            if let Some(handle) = process::current_process_handle() {
                process::inject_into_running_process(
                    handle,
                    &l_wrapper_path,
                    &l_addons_path,
                    &args,
                )?;

                return Ok(());
            } else {
                return Err(AppError::Message(format!(
                    "Dry run failed: cannot open current process '{}'",
                    DEFAULT_EXE_NAME
                )));
            }
        }

        if args.attach {
            if let Some(process_handle) = process::attach_to_process_by_name(DEFAULT_EXE_NAME) {
                process::inject_into_running_process(
                    process_handle.handle,
                    &l_wrapper_path,
                    &l_addons_path,
                    &args,
                )?;

                return Ok(());
            } else {
                return Err(AppError::Message(format!(
                    "Attach failed: process '{}' not running",
                    DEFAULT_EXE_NAME
                )));
            }
        }

        if args.no_inject {
            process::launch_without_injection(&l_exe_path, &args.game_args)?;
            return Ok(());
        }
    }

    process::launch_with_injection(&l_exe_path, &l_wrapper_path, &l_addons_path, args)
}
