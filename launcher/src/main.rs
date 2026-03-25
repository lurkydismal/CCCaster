use launcher::launcher_eprintln;
use std::error::Error;

fn main() -> Result<(), Box<dyn Error>> {
    dotenvy::dotenv()?;

    if let Err(l_err) = launcher::run_cli() {
        launcher_eprintln!("{l_err}");
        std::process::exit(1);
    }

    Ok(())
}
