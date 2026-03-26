use launcher::launcher_error;

fn main() {
    match dotenvy::dotenv() {
        Ok(_path) => {}
        Err(_e) => {}
    }

    if let Err(l_err) = launcher::run_cli() {
        launcher_error!("{l_err}");
        std::process::exit(1);
    }
}
