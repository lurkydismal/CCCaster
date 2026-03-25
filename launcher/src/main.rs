use launcher::launcher_error;
use std::error::Error;

fn main() -> Result<(), Box<dyn Error>> {
    match dotenvy::dotenv() {
        Ok(l_path) => {
            eprintln!(
                "[LAUNCHER] [DEBUG] Loaded environment file: {}",
                l_path.display()
            );
        }
        Err(l_err) => {
            let l_cwd = std::env::current_dir()
                .map(|l_path| l_path.display().to_string())
                .unwrap_or_else(|l_cwd_err| format!("<unavailable: {l_cwd_err}>"));
            eprintln!("[LAUNCHER] [ERROR] Failed to load .env (cwd: {l_cwd})");
            eprintln!("[LAUNCHER] [ERROR] dotenv error kind: {:?}", l_err);
            return Err(l_err.into());
        }
    }

    if let Err(l_err) = launcher::run_cli() {
        launcher_error!("{l_err}");
        std::process::exit(1);
    }

    Ok(())
}
