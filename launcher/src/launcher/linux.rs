use crate::{
    args::Args,
    error::{AppError, Result},
};

pub fn run(_args: &Args) -> Result<()> {
    Err(AppError::Unsupported(
        "this launcher implementation is Windows-only right now",
    ))
}
