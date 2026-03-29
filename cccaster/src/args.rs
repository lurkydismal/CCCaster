#[derive(Serialize, Debug)]
pub struct ModloaderData<'a> {
    // Core execution modes
    /// Default behavior. Launch + inject + run normally.
    pub play: &'a bool,

    /// Validate addons, dependency graph, patches, paths — do not launch.
    pub dry_run: &'a bool,

    /// Load only specific addons (override auto-load).
    pub addon: &'a Option<Vec<String>>,

    // Addon loading & resolution control
    /// Override default `addons/`.
    pub addons_dir: &'a Option<String>,

    /// Explicit load order override (bypass dependency resolver).
    pub load_order: &'a Option<String>,

    /// Ignore dependencies (dangerous but useful for debugging).
    pub no_deps: &'a bool,

    /// Ignore version/ API mismatches.
    pub force: &'a bool,

    /// Blacklist specific addons.
    pub disable: &'a Option<Vec<String>>,

    // Debugging & diagnostics
    /// Increase logging verbosity (-v, -vv, -vvv).
    /// Each additional `-v` increases detail level.
    pub verbose: &'a u8,

    /// Very noisy: patching, hooks, loader internals.
    pub trace: &'a bool,

    /// Output resolved patches after dependency resolution.
    pub dump_patches: &'a bool,

    /// Output mod dependency graph.
    pub dump_graph: &'a bool,

    /// Measure load/ injection phases.
    pub timings: &'a bool,

    // Safety/ isolation controls
    /// Disable all mods except core/ runtime.
    pub safe_mode: &'a bool,

    /// Restrict file access (NOTE: stub, no VFS yet).
    pub sandbox: &'a bool,

    /// Load mods but don’t apply binary patches (script-only testing).
    pub no_patches: &'a bool,
}
