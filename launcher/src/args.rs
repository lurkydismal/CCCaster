use clap::Parser;

#[derive(Parser, Clone, Debug)]
#[command(
    version,
    about,
    long_about = color_print::cstr!(r#"<bold><underline>Launcher Overview</underline></bold>

This tool controls how the game is started with addon support enabled. It handles discovery, validation, dependency resolution, and optional patching before launching the process.

<bold>Execution flow:</bold>
<dim>1.</dim> Parse arguments  
<dim>2.</dim> Discover addons  
<dim>3.</dim> Validate dependencies and compatibility  
<dim>4.</dim> Resolve load order  
<dim>5.</dim> Apply patches and prepare runtime  
<dim>6.</dim> Launch the game process  
<dim>7.</dim> Inject wrapper and transfer control  

<bold>Key capabilities:</bold>
<dim>•</dim> Selective addon loading and profiles  
<dim>•</dim> Strict or relaxed validation modes  
<dim>•</dim> Full control over load order and dependencies  
<dim>•</dim> Injection and runtime hooking via wrapper  
<dim>•</dim> Debugging and tracing tools  
<dim>•</dim> Safe, dry-run, and validation-only execution  

<bold>Usage examples:</bold>

<dim>$</dim> <bold>launcher --profile modded</bold>  
<dim>$</dim> <bold>launcher --validate</bold>  
<dim>$</dim> <bold>launcher --no-inject -- game.exe -fullscreen</bold>

<bold>Notes:</bold>
<dim>•</dim> Use <bold>--</bold> to pass arguments directly to the game  
<dim>•</dim> Modes like <bold>--dry-run</bold> do not start the game  
<dim>•</dim> Injection behavior can be controlled or disabled  
"#)
)]
pub struct Args {
    // Core execution modes
    /// Default behavior. Launch + inject + run normally.
    #[arg(long)]
    pub play: bool,

    /// Launch game without touching it (baseline comparison, debugging crashes).
    #[arg(long)]
    pub no_inject: bool,

    /// Validate addons, dependency graph, patches, paths — do not launch.
    #[arg(long)]
    pub dry_run: bool,

    /// Same as dry-run but stricter: checksum files, detect conflicts, ABI mismatches.
    #[arg(long)]
    pub validate: bool,

    /// Load only specific addons (override auto-load).
    #[arg(short, long, value_name = "NAME", action = clap::ArgAction::Append)]
    pub addon: Option<Vec<String>>,

    /// Load a predefined addon set.
    #[arg(short, long, value_name = "NAME")]
    pub profile: Option<String>,

    // Addon loading & resolution control
    /// Override default `addons/`.
    #[arg(long, value_name = "PATH")]
    pub addons_dir: Option<String>,

    /// Explicit load order override (bypass dependency resolver).
    #[arg(long, value_name = "FILE")]
    pub load_order: Option<String>,

    /// Ignore dependencies (dangerous but useful for debugging).
    #[arg(long)]
    pub no_deps: bool,

    /// Ignore version/ API mismatches.
    #[arg(short, long)]
    pub force: bool,

    /// Blacklist specific addons.
    #[arg(short, long, value_name = "ADDON", action = clap::ArgAction::Append)]
    pub disable: Option<Vec<String>>,

    // Injection/ runtime control
    /// Custom wrapper DLL.
    #[arg(short, long, value_name = "PATH")]
    pub wrapper: Option<String>,

    /// Fail if wrapper doesn’t signal readiness.
    #[arg(short, long, value_name = "MILLISECONDS")]
    pub inject_timeout: Option<usize>,

    /// Resume immediately after injection (useful if wrapper is optional).
    #[arg(long)]
    pub no_wait_wrapper: bool,

    /// Keep process suspended after injection (for manual debugging with external tools).
    #[arg(long)]
    pub suspend: bool,

    /// Attach to an already running process instead of spawning.
    #[arg(long)]
    pub attach: bool,

    // Debugging & diagnostics
    /// Increase logging verbosity (-v, -vv, -vvv).
    /// Each additional `-v` increases detail level.
    #[arg(short, long, action = clap::ArgAction::Count)]
    pub verbose: u8,

    /// Very noisy: patching, hooks, loader internals.
    #[arg(long)]
    pub trace: bool,

    /// Output resolved patches after dependency resolution.
    #[arg(long)]
    pub dump_patches: bool,

    /// Output mod dependency graph.
    #[arg(long)]
    pub dump_graph: bool,

    /// Measure load/ injection phases.
    #[arg(long)]
    pub timings: bool,

    /// Break before resuming process (for debugger).
    #[arg(short, long)]
    pub break_on_load: bool,

    // Safety/ isolation controls
    /// Disable all mods except core/ runtime.
    #[arg(long)]
    pub safe_mode: bool,

    /// Restrict file access (NOTE: stub, no VFS yet).
    #[arg(short, long)]
    pub sandbox: bool,

    /// Load mods but don’t apply binary patches (script-only testing).
    #[arg(long)]
    pub no_patches: bool,

    // Game argument passthrough
    /// Everything after goes directly to the game executable.
    #[arg(trailing_var_arg = true, allow_hyphen_values = true)]
    pub game_args: Vec<String>,
}
