use serde::{Deserialize, Serialize};

/// Launcher-provided runtime configuration for the modloader.
#[derive(Serialize, Deserialize, Clone, Debug)]
pub struct ModloaderData {
    // Core execution modes
    /// Default behavior. Launch + inject + run normally.
    pub play: bool,

    /// Validate addons, dependency graph, patches, paths — do not launch.
    pub dry_run: bool,

    /// Load only specific addons (override auto-load).
    pub addon: Option<Vec<String>>,

    // Addon loading & resolution control
    /// Override default `addons/`.
    pub addons_dir: Option<String>,

    /// Explicit load order override (bypass dependency resolver).
    pub load_order: Option<String>,

    /// Ignore dependencies (dangerous but useful for debugging).
    pub no_deps: bool,

    /// Ignore version/ API mismatches.
    pub force: bool,

    /// Blacklist specific addons.
    pub disable: Option<Vec<String>>,

    // Debugging & diagnostics
    /// Increase logging verbosity (-v, -vv, -vvv).
    /// Each additional `-v` increases detail level.
    pub verbose: u8,

    /// Very noisy: patching, hooks, loader internals.
    pub trace: bool,

    /// Output resolved patches after dependency resolution.
    pub dump_patches: bool,

    /// Output mod dependency graph.
    pub dump_graph: bool,

    /// Measure load/ injection phases.
    pub timings: bool,

    // Safety/ isolation controls
    /// Disable all mods except core/ runtime.
    pub safe_mode: bool,

    /// Restrict global file access.
    pub sandbox: bool,
}
