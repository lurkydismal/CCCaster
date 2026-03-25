use crate::{
    args::Args,
    error::{AppError, Result},
};
use std::path::Path;

pub fn run(_args: &Args, _root_path: &Path) -> Result<()> {
    crate::launcher_trace!("linux::run accepted args");
    Err(AppError::Unsupported(
        "this launcher implementation is Windows-only right now",
    ))
}
