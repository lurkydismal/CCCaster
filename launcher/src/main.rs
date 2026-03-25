use launcher::launcher_error;
use std::error::Error;

fn main() -> Result<(), Box<dyn Error>> {
    dotenvy::dotenv()?;

    if let Err(l_err) = launcher::run_cli() {
        launcher_error!("{l_err}");
        std::process::exit(1);
    }

    Ok(())
}
