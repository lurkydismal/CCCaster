use launcher::launcher_eprintln;

fn main() {
    if let Err(l_err) = launcher::run_cli() {
        launcher_eprintln!("{l_err}");
        std::process::exit(1);
    }
}
