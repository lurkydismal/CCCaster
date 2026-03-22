fn main() {
    if let Err(l_err) = launcher::run_cli() {
        eprintln!("{l_err}");
        std::process::exit(1);
    }
}
