use crate::api::make_patch;
use crate::patch::{
    OwnedPatchSpan, PatchEntry, PatchSpan, Patches, ResolvedPatch, ensure_no_overlap,
    resolve_patch_entries, spans_for_patches,
};
use crate::types::{Dependency, ModMeta, RawModInfo};
use crate::{
    LOG_DEBUG, LOG_ERROR, LOG_INFO, LOG_TRACE, LOG_WARNING, modloader_debug, modloader_error,
    modloader_info, modloader_trace, modloader_warning, runtime_args,
};
use anyhow::{Context, Result, anyhow};
use blake3::Hash;
use mlua::{Function, Lua, Table, Value};
use notify::{Event, EventKind, RecommendedWatcher, RecursiveMode, Watcher};
use once_cell::sync::Lazy;
use petgraph::algo::toposort;
use petgraph::graph::Graph;
use semver::{Version, VersionReq};
use std::collections::{HashMap, HashSet};
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant};
use tokio::fs;
use tokio::sync::oneshot;

type StartupSignal = Arc<Mutex<Option<oneshot::Sender<Result<()>>>>>;
type ShutdownSender = std::sync::mpsc::Sender<ControlMessage>;

enum ControlMessage {
    Shutdown,
}

static HOT_RELOAD_THREAD: Lazy<Mutex<Option<JoinHandle<()>>>> = Lazy::new(|| Mutex::new(None));
static SHUTDOWN_SIGNAL: Lazy<Mutex<Option<ShutdownSender>>> = Lazy::new(|| Mutex::new(None));

struct LoadedMod {
    /// Fully parsed metadata from `info.json`.
    meta: ModMeta,
    /// Absolute path to the addon directory.
    path: PathBuf,
    /// Active patch set for this mod.
    _patches: Option<Patches>,
    /// Active patch ranges for overlap detection.
    patch_spans: Vec<PatchSpan>,
    /// Files (and last hash) used for hot-reload invalidation.
    watched_hashes: HashMap<PathBuf, Option<Hash>>,
}

/// Scans the addons directory, initializes mods, and starts hot-reload in a background thread.
pub async fn load_mods_from_addons() -> Result<()> {
    let (ready_tx, ready_rx) = oneshot::channel::<Result<()>>();
    let (control_tx, control_rx) = std::sync::mpsc::channel::<ControlMessage>();
    let ready_signal = Arc::new(Mutex::new(Some(ready_tx)));
    let thread_signal = ready_signal.clone();

    let hot_reload_thread = thread::Builder::new()
        .name("cccaster-hot-reload".to_string())
        .spawn(move || {
            let runtime = match tokio::runtime::Builder::new_current_thread()
                .enable_all()
                .build()
            {
                Ok(runtime) => runtime,
                Err(err) => {
                    send_startup_signal(
                        &thread_signal,
                        Err(anyhow!("failed to create hot-reload runtime: {}", err)),
                    );
                    return;
                }
            };

            if let Err(err) = runtime.block_on(load_mods_from_addons_async(
                thread_signal.clone(),
                control_rx,
            )) {
                modloader_error!("Modloader runtime exited with error: {}", err);
                send_startup_signal(&thread_signal, Err(err));
            }
        })
        .map_err(|err| anyhow!("failed to spawn hot-reload thread: {}", err))?;

    {
        let mut shutdown_guard = SHUTDOWN_SIGNAL
            .lock()
            .map_err(|_| anyhow!("shutdown signal mutex poisoned"))?;
        *shutdown_guard = Some(control_tx);
    }
    {
        let mut thread_guard = HOT_RELOAD_THREAD
            .lock()
            .map_err(|_| anyhow!("hot-reload thread mutex poisoned"))?;
        *thread_guard = Some(hot_reload_thread);
    }

    ready_rx
        .await
        .map_err(|err| anyhow!("failed waiting for modloader startup: {}", err))?
}

/// Scans the 'addons' directory, loads mods, resolves dependencies, and initializes mods.
async fn load_mods_from_addons_async(
    ready_signal: StartupSignal,
    control_rx: std::sync::mpsc::Receiver<ControlMessage>,
) -> Result<()> {
    let args = runtime_args();
    let startup_started = Instant::now();
    // FIX: Somehow resolve windows path here or in launcher
    // let addons_dir = PathBuf::from(args.addons_dir.as_deref().unwrap_or("addons"));
    // let addons_dir_path = addons_dir.as_path();
    let addons_dir_path = Path::new("addons");
    let addon_filter: Option<HashSet<&str>> = args
        .addon
        .as_ref()
        .map(|entries| entries.iter().map(String::as_str).collect());
    let disable_filter: HashSet<&str> = args
        .disable
        .as_ref()
        .map(|entries| entries.iter().map(String::as_str).collect())
        .unwrap_or_default();
    let mut mod_entries: Vec<(ModMeta, PathBuf)> = Vec::new();

    modloader_info!("Starting mod discovery in {:?}", addons_dir_path);

    let mut dir = fs::read_dir(addons_dir_path)
        .await
        .with_context(|| format!("unable to open addons directory at {:?}", addons_dir_path))?;
    while let Some(entry) = dir
        .next_entry()
        .await
        .with_context(|| format!("failed while iterating entries in {:?}", addons_dir_path))?
    {
        let path = entry.path();
        modloader_trace!("Inspecting addon entry at {:?}", path);
        if path.is_dir() {
            if !is_safe_path(addons_dir_path, &path) {
                modloader_warning!("Skipping unsafe directory path: {:?}", path);
                continue;
            }
            let mod_id = match derive_mod_id(&path) {
                Some(id) => id,
                None => {
                    modloader_warning!(
                        "Skipping addon path with invalid directory name: {:?}",
                        path
                    );
                    continue;
                }
            };
            if args.safe_mode && mod_id != "core" && mod_id != "runtime" {
                modloader_debug!("safe_mode skipping addon '{}'", mod_id);
                continue;
            }
            if disable_filter.contains(mod_id.as_str()) {
                modloader_debug!("Skipping disabled addon '{}'", mod_id);
                continue;
            }
            if let Some(filter) = addon_filter.as_ref()
                && !filter.contains(mod_id.as_str())
            {
                modloader_trace!("Skipping '{}' because it is not in --addon filter", mod_id);
                continue;
            }

            let info_path = path.join("info.json");
            let info_data = fs::read(&info_path).await;
            if let Err(err) = info_data.as_ref() {
                modloader_error!(
                    "Failed to read mod manifest {:?} for {:?}: {}. Skipping this addon.",
                    info_path,
                    path,
                    err
                );
                continue;
            }
            let raw: RawModInfo = match serde_json::from_slice(&info_data.unwrap()) {
                Ok(raw) => raw,
                Err(err) => {
                    modloader_error!(
                        "Failed to parse manifest {:?} for {:?}: {}. Skipping this addon.",
                        info_path,
                        path,
                        err
                    );
                    continue;
                }
            };

            let version = match Version::parse(&raw.version) {
                Ok(version) => version,
                Err(err) => {
                    modloader_error!(
                        "Invalid version '{}' in {:?} for mod {}: {}. Skipping this addon.",
                        raw.version,
                        info_path,
                        mod_id,
                        err
                    );
                    continue;
                }
            };

            let mut deps = Vec::new();
            let mut dep_parse_failed = false;
            for rd in raw.dependencies {
                let ver_req = match VersionReq::parse(&rd.version_req) {
                    Ok(ver_req) => ver_req,
                    Err(err) => {
                        modloader_error!(
                            "Invalid dependency version requirement '{}' in {:?} (mod {}, dependency {}): {}. Skipping this addon.",
                            rd.version_req,
                            info_path,
                            mod_id,
                            rd.id,
                            err
                        );
                        dep_parse_failed = true;
                        break;
                    }
                };
                deps.push(Dependency {
                    id: rd.id,
                    version_req: ver_req,
                    optional: rd.optional,
                });
            }
            if dep_parse_failed {
                continue;
            }

            mod_entries.push((
                ModMeta {
                    id: mod_id.clone(),
                    version,
                    dependencies: deps,
                    api_version: raw.api_version,
                    events: raw.events.clone(),
                },
                path.clone(),
            ));
            modloader_debug!("Registered addon candidate '{}' from {:?}", mod_id, path);
        }
    }

    modloader_info!(
        "Discovered {} addon candidates; resolving dependency load order",
        mod_entries.len()
    );
    let load_order = resolve_load_order(&mod_entries, args.no_deps)?;
    if args.dump_graph {
        for &mod_index in &load_order {
            let (meta, _path) = &mod_entries[mod_index];
            let deps: Vec<String> = meta
                .dependencies
                .iter()
                .map(|dep| format!("{}{}", dep.id, if dep.optional { "?" } else { "" }))
                .collect();
            modloader_info!("graph: {} -> [{}]", meta.id, deps.join(", "));
        }
    }
    if let Some(load_order_override) = args.load_order.as_deref() {
        let explicit_order =
            resolve_explicit_load_order(load_order_override, &mod_entries, args.force)?;
        modloader_info!(
            "Applying explicit load order override ({} entries)",
            explicit_order.len()
        );
        return run_with_load_order(
            ready_signal,
            &mod_entries,
            &explicit_order,
            args.dry_run || !args.play,
            args.timings,
            startup_started,
            control_rx,
        )
        .await;
    }
    modloader_debug!("Resolved load order indexes: {:?}", load_order);
    run_with_load_order(
        ready_signal,
        &mod_entries,
        &load_order,
        args.dry_run || !args.play,
        args.timings,
        startup_started,
        control_rx,
    )
    .await
}

async fn run_with_load_order(
    ready_signal: StartupSignal,
    mod_entries: &[(ModMeta, PathBuf)],
    load_order: &[usize],
    dry_run: bool,
    timings: bool,
    startup_started: Instant,
    control_rx: std::sync::mpsc::Receiver<ControlMessage>,
) -> Result<()> {
    if dry_run {
        modloader_info!(
            "dry_run enabled; validated {} mods without runtime initialization",
            load_order.len()
        );
        send_startup_signal(&ready_signal, Ok(()));
        return Ok(());
    }

    let args = runtime_args();
    modloader_info!("Initializing Lua runtime with sandbox enabled");
    let lua = Lua::new();
    lua.sandbox(true)?;
    let globals = lua.globals();
    let engine_table = lua.create_table()?;
    install_engine_log_api(&lua, &engine_table)?;
    install_engine_memory_api(&lua, &engine_table)?;
    install_engine_dispatch_api(&lua, &engine_table, load_order, mod_entries)?;
    globals.set("Engine", engine_table)?;

    let mut loaded_mods: Vec<LoadedMod> = Vec::new();
    for &mod_index in load_order {
        let (meta, path) = &mod_entries[mod_index];
        modloader_info!("Loading mod '{}' from {:?}", meta.id, path);
        let occupied = collect_occupied_spans(&loaded_mods, None);
        let loaded = load_mod(
            &lua,
            meta.clone(),
            path.clone(),
            false,
            Value::Nil,
            &occupied,
            args.dump_patches,
        )
        .await
        .with_context(|| format!("initial load failed for mod '{}'", meta.id))?;
        loaded_mods.push(loaded);
    }

    modloader_info!("All mods loaded; invoking post_init callbacks");
    call_post_init_callbacks(&lua, load_order, mod_entries)?;
    if timings {
        modloader_trace!("Startup completed in {:?}", startup_started.elapsed());
    }
    start_hot_reload_loop(lua, loaded_mods, ready_signal, control_rx).await
}

async fn start_hot_reload_loop(
    lua: Lua,
    mut loaded_mods: Vec<LoadedMod>,
    ready_signal: StartupSignal,
    control_rx: std::sync::mpsc::Receiver<ControlMessage>,
) -> Result<()> {
    let (tx, rx) = std::sync::mpsc::channel::<notify::Result<Event>>();
    let mut watcher: RecommendedWatcher = notify::recommended_watcher(move |event| {
        let _ = tx.send(event);
    })?;

    for loaded in &loaded_mods {
        watcher.watch(&loaded.path, RecursiveMode::Recursive)?;
    }

    modloader_info!("Hot-reload watcher started for {} mods", loaded_mods.len());
    send_startup_signal(&ready_signal, Ok(()));

    loop {
        if let Ok(message) = control_rx.try_recv() {
            match message {
                ControlMessage::Shutdown => {
                    modloader_info!("Shutdown signal received; invoking quit callbacks");
                    call_quit_callbacks(&lua, &loaded_mods)?;
                    return Ok(());
                }
            }
        }

        let event = match rx.recv_timeout(Duration::from_millis(250)) {
            Ok(Ok(event)) => event,
            Ok(Err(err)) => {
                modloader_warning!("File watcher reported an error: {}", err);
                continue;
            }
            Err(std::sync::mpsc::RecvTimeoutError::Timeout) => continue,
            Err(std::sync::mpsc::RecvTimeoutError::Disconnected) => {
                return Err(anyhow!("hot-reload file watcher channel disconnected"));
            }
        };
        modloader_trace!(
            "Watcher event received: kind={:?}, paths={:?}",
            event.kind,
            event.paths
        );

        if !matches!(
            event.kind,
            EventKind::Modify(_) | EventKind::Create(_) | EventKind::Remove(_)
        ) {
            continue;
        }

        let mut mods_to_reload: HashSet<usize> = HashSet::new();
        for path in &event.paths {
            if let Some(changed_index) = detect_changed_mod(&mut loaded_mods, path) {
                mods_to_reload.insert(changed_index);
            }
        }

        for mod_index in mods_to_reload {
            let meta = loaded_mods[mod_index].meta.clone();
            let path = loaded_mods[mod_index].path.clone();

            if let Err(err) = hot_reload_mod(&lua, &mut loaded_mods, mod_index).await {
                modloader_error!("Hot-reload failed for mod {}: {}", meta.id, err);
            } else {
                modloader_info!("Hot-reloaded mod {} from {:?}", meta.id, path);
            }
        }
    }
}

pub fn shutdown_before_unload() {
    let tx = match SHUTDOWN_SIGNAL.lock() {
        Ok(mut guard) => guard.take(),
        Err(_) => {
            modloader_error!("Failed to lock shutdown signal for unload");
            None
        }
    };
    if let Some(tx) = tx
        && let Err(err) = tx.send(ControlMessage::Shutdown)
    {
        modloader_warning!("Failed to signal hot-reload shutdown: {}", err);
    }

    let thread_handle = match HOT_RELOAD_THREAD.lock() {
        Ok(mut guard) => guard.take(),
        Err(_) => {
            modloader_error!("Failed to lock hot-reload thread handle for unload");
            None
        }
    };
    if let Some(handle) = thread_handle
        && let Err(_panic) = handle.join()
    {
        modloader_error!("Hot-reload thread panicked while shutting down");
    }
}

fn send_startup_signal(ready_signal: &StartupSignal, result: Result<()>) {
    if let Ok(mut guard) = ready_signal.lock()
        && let Some(sender) = guard.take()
    {
        let _ = sender.send(result);
    }
}

fn derive_mod_id(path: &Path) -> Option<String> {
    path.file_name()
        .and_then(|name| name.to_str())
        .map(|name| name.replace(' ', "_"))
}

/// Determines whether a watched file changed content for any loaded mod.
fn detect_changed_mod(loaded_mods: &mut [LoadedMod], path: &Path) -> Option<usize> {
    let normalized = normalize_path(path);
    for (idx, loaded) in loaded_mods.iter_mut().enumerate() {
        if let Some(previous_hash) = loaded.watched_hashes.get(&normalized).copied() {
            let current_hash = hash_file(&normalized);
            if previous_hash != current_hash {
                modloader_debug!(
                    "Detected actual content change for mod {} in {:?}",
                    loaded.meta.id,
                    normalized
                );
                return Some(idx);
            }
        }
    }
    None
}

fn collect_occupied_spans<'a>(
    loaded_mods: &'a [LoadedMod],
    skip_mod_index: Option<usize>,
) -> Vec<OwnedPatchSpan<'a>> {
    let mut occupied = Vec::new();
    for (idx, loaded) in loaded_mods.iter().enumerate() {
        if skip_mod_index.is_some_and(|skip| skip == idx) {
            continue;
        }
        for span in &loaded.patch_spans {
            occupied.push(OwnedPatchSpan {
                owner: loaded.meta.id.as_str(),
                span: *span,
            });
        }
    }
    occupied
}

async fn hot_reload_mod(lua: &Lua, loaded_mods: &mut [LoadedMod], mod_index: usize) -> Result<()> {
    let args = runtime_args();
    modloader_info!(
        "Starting hot-reload for mod {}",
        loaded_mods[mod_index].meta.id
    );
    let unload_payload = run_unload(lua, &loaded_mods[mod_index].meta.id)?;
    let occupied = collect_occupied_spans(loaded_mods, Some(mod_index));
    let reloaded = load_mod(
        lua,
        loaded_mods[mod_index].meta.clone(),
        loaded_mods[mod_index].path.clone(),
        true,
        unload_payload,
        &occupied,
        args.dump_patches,
    )
    .await?;
    loaded_mods[mod_index] = reloaded;
    modloader_info!(
        "Finished hot-reload for mod {}",
        loaded_mods[mod_index].meta.id
    );
    Ok(())
}

/// Invokes a mod's optional `unload()` callback and returns its payload.
fn run_unload(lua: &Lua, mod_id: &str) -> Result<Value> {
    let engine_table: Table = lua.globals().get("Engine")?;
    let existing_mod: Table = engine_table
        .get(mod_id)
        .with_context(|| format!("mod table '{}' is not registered in Engine", mod_id))?;

    if let Ok(unload) = existing_mod.get::<Function>("unload") {
        modloader_debug!("Calling unload() for mod {}", mod_id);
        return unload
            .call::<Value>(())
            .with_context(|| format!("unload() failed for mod {}", mod_id));
    }

    Ok(Value::Nil)
}

async fn load_mod(
    lua: &Lua,
    meta: ModMeta,
    path: PathBuf,
    is_hot_reload: bool,
    unload_payload: Value,
    occupied_spans: &[OwnedPatchSpan<'_>],
    dump_patches: bool,
) -> Result<LoadedMod> {
    modloader_debug!(
        "Loading mod '{}' (hot_reload={}) from {:?}",
        meta.id,
        is_hot_reload,
        path
    );
    let required_files = Arc::new(Mutex::new(HashSet::new()));
    register_engine_require(lua, path.clone(), required_files.clone())
        .with_context(|| format!("failed to register Engine.require for mod {}", meta.id))?;

    let patch_path = path.join("patch.json");
    let has_patch_file = patch_path.exists();
    let main_path = path.join("main.luau");
    let has_main_script = main_path.exists();

    if !has_patch_file && !has_main_script {
        return Err(anyhow!(
            "mod {} must provide at least one of main.luau or patch.json",
            meta.id
        ));
    }

    let (patches, patch_spans) = if has_patch_file {
        modloader_trace!("Found patch file for mod {} at {:?}", meta.id, patch_path);
        let patch_data = fs::read(&patch_path).await.with_context(|| {
            format!(
                "failed to read patch file {:?} for mod {}",
                patch_path, meta.id
            )
        })?;
        let patch_entries: Vec<PatchEntry> =
            serde_json::from_slice(&patch_data).with_context(|| {
                format!(
                    "failed to parse patch file {:?} for mod {}",
                    patch_path, meta.id
                )
            })?;
        let resolved_entries: Vec<ResolvedPatch> = resolve_patch_entries(&patch_entries, &meta.id)?;
        let spans = spans_for_patches(&resolved_entries)?;
        ensure_no_overlap(&meta.id, &spans, occupied_spans)?;
        if dump_patches {
            for (idx, patch) in resolved_entries.iter().enumerate() {
                modloader_info!(
                    "patch[{}] {} => 0x{:X} ({} bytes)",
                    idx,
                    meta.id,
                    patch.address,
                    patch.bytes.len()
                );
            }
        }
        modloader_info!(
            "Applying {} patch entries for mod {}",
            resolved_entries.len(),
            meta.id
        );
        (Some(Patches::new(&resolved_entries)), spans)
    } else {
        modloader_trace!("No patch.json present for mod {}", meta.id);
        (None, Vec::new())
    };

    let returned: Table = if has_main_script {
        let script = fs::read_to_string(&main_path).await.with_context(|| {
            format!(
                "failed to read main script {:?} for mod {}",
                main_path, meta.id
            )
        })?;

        lua.load(&script).eval().with_context(|| {
            format!(
                "failed to evaluate Lua script {:?} for mod {}",
                main_path, meta.id
            )
        })?
    } else {
        modloader_info!(
            "Mod {} has no main.luau; loading patches-only addon",
            meta.id
        );
        lua.create_table()?
    };

    let engine_table: Table = lua.globals().get("Engine")?;
    engine_table.set(meta.id.clone(), returned.clone())?;
    modloader_trace!("Registered Engine['{}'] table", meta.id);

    if is_hot_reload {
        if let Ok(load) = returned.get::<Function>("load") {
            modloader_debug!("Calling load() for mod {}", meta.id);
            if matches!(unload_payload, Value::Nil) {
                load.call::<()>(())
                    .with_context(|| format!("load() failed for mod {}", meta.id))?;
            } else {
                load.call::<()>(unload_payload)
                    .with_context(|| format!("load() failed for mod {}", meta.id))?;
            }
        }
    } else if let Ok(init_fn) = returned.get::<Function>("init") {
        modloader_debug!("Calling init() for mod {}", meta.id);
        if let Err(err) = init_fn.call::<()>(()) {
            modloader_error!("init() failed for mod {}: {}", meta.id, err);
        }
    }

    let mut watched_hashes = HashMap::new();
    for watched_path in build_watched_files(&path, &required_files) {
        watched_hashes.insert(watched_path.clone(), hash_file(&watched_path));
    }
    modloader_debug!(
        "Tracking {} watched files for mod {}",
        watched_hashes.len(),
        meta.id
    );

    Ok(LoadedMod {
        meta,
        path,
        _patches: patches,
        patch_spans,
        watched_hashes,
    })
}

fn build_watched_files(
    mod_path: &Path,
    required_files: &Arc<Mutex<HashSet<PathBuf>>>,
) -> Vec<PathBuf> {
    // Core mod files are always watched.
    let mut files = vec![
        normalize_path(&mod_path.join("patch.json")),
        normalize_path(&mod_path.join("info.json")),
        normalize_path(&mod_path.join("main.luau")),
    ];

    if let Ok(guard) = required_files.lock() {
        files.extend(guard.iter().cloned());
    }

    files.sort();
    files.dedup();
    files
}

fn resolve_load_order(mod_entries: &[(ModMeta, PathBuf)], no_deps: bool) -> Result<Vec<usize>> {
    if no_deps {
        modloader_warning!("no_deps enabled; using filesystem discovery order");
        return Ok((0..mod_entries.len()).collect());
    }
    modloader_debug!(
        "Resolving dependency graph for {} mod entries",
        mod_entries.len()
    );
    let mut graph = Graph::<usize, ()>::new();
    let mut indices: HashMap<String, petgraph::graph::NodeIndex> = HashMap::new();
    for (i, (meta, _path)) in mod_entries.iter().enumerate() {
        indices.insert(meta.id.clone(), graph.add_node(i));
    }

    mod_entries.iter().for_each(|(meta, _path)| {
        for dep in &meta.dependencies {
            modloader_trace!(
                "Inspecting dependency edge: {} -> {} ({})",
                meta.id,
                dep.id,
                dep.version_req
            );
            if let Some(&dep_idx) = indices.get(&dep.id) {
                let target_index = *graph.node_weight(dep_idx).unwrap();
                let target_meta = &mod_entries[target_index].0;
                if dep.version_req.matches(&target_meta.version) {
                    let this_idx = indices[&meta.id];
                    graph.add_edge(dep_idx, this_idx, ());
                    modloader_trace!("Accepted dependency edge {} -> {}", dep.id, meta.id);
                } else if !dep.optional {
                    modloader_warning!(
                        "Dependency version mismatch: {} requires {}, found {}",
                        meta.id,
                        dep.version_req,
                        target_meta.version
                    );
                }
            } else if !dep.optional {
                modloader_warning!("Missing required dependency {} for mod {}", dep.id, meta.id);
            }
        }
    });

    let sorted = toposort(&graph, None).map_err(|cycle| {
        anyhow!(
            "Circular dependency detected involving index: {:?}",
            cycle.node_id()
        )
    })?;

    Ok(sorted
        .into_iter()
        .map(|idx| *graph.node_weight(idx).unwrap())
        .collect())
}

fn resolve_explicit_load_order(
    raw_order: &str,
    mod_entries: &[(ModMeta, PathBuf)],
    force: bool,
) -> Result<Vec<usize>> {
    let mut explicit = Vec::new();
    let mut consumed = HashSet::new();
    for token in raw_order.split(';') {
        let trimmed = token.trim();
        if trimmed.is_empty() {
            continue;
        }
        let (mod_id, version_raw) = trimmed
            .split_once('@')
            .ok_or_else(|| anyhow!("invalid load_order token '{}': missing '@version'", trimmed))?;
        if mod_id.trim().is_empty() || version_raw.trim().is_empty() {
            return Err(anyhow!(
                "invalid load_order token '{}': both mod and version are required",
                trimmed
            ));
        }
        let version_req = VersionReq::parse(version_raw.trim()).with_context(|| {
            format!(
                "invalid load_order version '{}' for mod '{}'",
                version_raw, mod_id
            )
        })?;
        let found = mod_entries
            .iter()
            .enumerate()
            .find(|(_idx, (meta, _path))| {
                meta.id == mod_id.trim() && version_req.matches(&meta.version)
            })
            .map(|(idx, _)| idx);
        let selected = if let Some(idx) = found {
            idx
        } else if force {
            mod_entries
                .iter()
                .enumerate()
                .find(|(_idx, (meta, _path))| meta.id == mod_id.trim())
                .map(|(idx, _)| idx)
                .ok_or_else(|| anyhow!("load_order references missing mod '{}'", mod_id.trim()))?
        } else {
            return Err(anyhow!(
                "load_order requires '{}' with version '{}', but no matching addon was found",
                mod_id.trim(),
                version_raw.trim()
            ));
        };
        if consumed.insert(selected) {
            explicit.push(selected);
        }
    }

    for idx in 0..mod_entries.len() {
        if consumed.insert(idx) {
            explicit.push(idx);
        }
    }
    Ok(explicit)
}

fn call_post_init_callbacks(
    lua: &Lua,
    load_order: &[usize],
    mod_entries: &[(ModMeta, PathBuf)],
) -> Result<()> {
    let engine_table: Table = lua.globals().get("Engine")?;
    for &mod_index in load_order {
        let (meta, _path) = &mod_entries[mod_index];
        let mod_table: Table = engine_table.get(meta.id.clone())?;
        if let Ok(post_fn) = mod_table.get::<Function>("post_init") {
            modloader_debug!("Calling post_init() for mod {}", meta.id);
            if let Err(err) = post_fn.call::<()>(()) {
                modloader_error!("post_init() failed for mod {}: {}", meta.id, err);
            }
        }
    }
    Ok(())
}

fn call_quit_callbacks(lua: &Lua, loaded_mods: &[LoadedMod]) -> Result<()> {
    let engine_table: Table = lua.globals().get("Engine")?;
    for loaded in loaded_mods.iter().rev() {
        let mod_table: Table = engine_table.get(loaded.meta.id.clone())?;
        if let Ok(quit_fn) = mod_table.get::<Function>("quit") {
            modloader_debug!("Calling quit() for mod {}", loaded.meta.id);
            if let Err(err) = quit_fn.call::<()>(()) {
                modloader_error!("quit() failed for mod {}: {}", loaded.meta.id, err);
            }
        }
    }
    Ok(())
}

fn install_engine_log_api(lua: &Lua, engine_table: &Table) -> Result<()> {
    let log_fn = lua.create_function(|_, (level, message): (u8, String)| {
        match level {
            LOG_ERROR => modloader_error!("{}", message),
            LOG_WARNING => modloader_warning!("{}", message),
            LOG_INFO => modloader_info!("{}", message),
            LOG_DEBUG => modloader_debug!("{}", message),
            LOG_TRACE => modloader_trace!("{}", message),
            other => modloader_warning!(
                "Engine.log received unsupported level {} with message: {}",
                other,
                message
            ),
        }
        Ok(())
    })?;
    engine_table.set("log", log_fn)?;
    engine_table.set("LOG_ERROR", LOG_ERROR)?;
    engine_table.set("LOG_WARNING", LOG_WARNING)?;
    engine_table.set("LOG_INFO", LOG_INFO)?;
    engine_table.set("LOG_DEBUG", LOG_DEBUG)?;
    engine_table.set("LOG_TRACE", LOG_TRACE)?;
    Ok(())
}

fn install_engine_memory_api(lua: &Lua, engine_table: &Table) -> Result<()> {
    let memory_table = lua.create_table()?;

    let read_fn = lua.create_function(|lua, (address, length): (Value, usize)| {
        let addr = match parse_lua_address(address) {
            Ok(value) => value,
            Err(err) => {
                modloader_warning!("Engine.memory.read failed: {err}");
                return Ok(Value::Nil);
            }
        };

        let end = match addr.checked_add(length) {
            Some(value) => value,
            None => {
                modloader_warning!(
                    "Engine.memory.read failed: address overflow for range 0x{:X}..+{}",
                    addr,
                    length
                );
                return Ok(Value::Nil);
            }
        };
        if !is_probably_readable(addr, length) {
            modloader_warning!(
                "Engine.memory.read failed: unreadable range 0x{:X}..0x{:X}",
                addr,
                end
            );
            return Ok(Value::Nil);
        }

        let out = lua.create_table()?;
        for idx in 0..length {
            let byte = unsafe { ((addr + idx) as *const u8).read() };
            out.set(idx + 1, format!("{:02X}", byte))?;
        }
        Ok(Value::Table(out))
    })?;

    let write_fn = lua.create_function(|_, (address, bytes): (Value, String)| {
        let addr = match parse_lua_address(address) {
            Ok(value) => value,
            Err(err) => {
                modloader_warning!("Engine.memory.write failed: {err}");
                return Ok(false);
            }
        };

        let parsed = match parse_hex_bytes_string(&bytes) {
            Ok(value) => value,
            Err(err) => {
                modloader_warning!("Engine.memory.write failed: {err}");
                return Ok(false);
            }
        };
        if parsed.is_empty() {
            modloader_warning!("Engine.memory.write failed: byte payload cannot be empty");
            return Ok(false);
        }

        let end = match addr.checked_add(parsed.len()) {
            Some(value) => value,
            None => {
                modloader_warning!(
                    "Engine.memory.write failed: address overflow for range 0x{:X}..+{}",
                    addr,
                    parsed.len()
                );
                return Ok(false);
            }
        };

        if !is_probably_writable(addr, parsed.len()) {
            modloader_warning!(
                "Engine.memory.write failed: unwritable range 0x{:X}..0x{:X}",
                addr,
                end
            );
            return Ok(false);
        }

        unsafe {
            std::ptr::copy_nonoverlapping(parsed.as_ptr(), addr as *mut u8, parsed.len());
        }
        Ok(true)
    })?;

    let patch_fn = lua.create_function(|lua, args: mlua::MultiValue| {
        let patches = parse_script_patch_args(args)?;
        if patches.is_empty() {
            modloader_warning!("Engine.memory.patch failed: no patch entries provided");
            return Ok(Value::Nil);
        }

        let mut handles: Vec<u32> = Vec::new();
        for patch in patches {
            let end = match patch.address.checked_add(patch.bytes.len()) {
                Some(value) => value,
                None => {
                    modloader_warning!(
                        "Engine.memory.patch failed: address overflow at 0x{:X}",
                        patch.address
                    );
                    return Ok(Value::Nil);
                }
            };

            if !is_probably_writable(patch.address, patch.bytes.len()) {
                modloader_warning!(
                    "Engine.memory.patch failed: unwritable range 0x{:X}..0x{:X}",
                    patch.address,
                    end
                );
                return Ok(Value::Nil);
            }

            let handle = make_patch(patch.address, &patch.bytes);
            handles.push(handle);
        }

        if handles.len() == 1 {
            return Ok(Value::Integer(handles[0] as i32));
        }

        let out = lua.create_table()?;
        for (idx, handle) in handles.iter().enumerate() {
            out.set(idx + 1, *handle)?;
        }
        Ok(Value::Table(out))
    })?;

    memory_table.set("read", read_fn)?;
    memory_table.set("write", write_fn)?;
    memory_table.set("patch", patch_fn)?;
    engine_table.set("memory", memory_table)?;
    Ok(())
}

struct ScriptPatch {
    address: usize,
    bytes: Vec<u8>,
}

fn parse_script_patch_args(args: mlua::MultiValue) -> mlua::Result<Vec<ScriptPatch>> {
    if args.len() == 1
        && let Some(Value::Table(table)) = args.front()
    {
        return parse_script_patch_table(table.clone());
    }

    let values: Vec<Value> = args.into_iter().collect();
    let mut out = Vec::new();
    let mut idx = 0usize;
    while idx < values.len() {
        if idx + 1 >= values.len() {
            return Err(mlua::Error::runtime(
                "Engine.memory.patch expects (address, bytes[, pattern]) groups",
            ));
        }
        let address = parse_lua_address(values[idx].clone()).map_err(mlua::Error::runtime)?;
        let bytes_text = value_as_string(values[idx + 1].clone())?;
        let bytes = parse_hex_bytes_string(&bytes_text).map_err(mlua::Error::runtime)?;

        if idx + 2 < values.len()
            && let Ok(pattern_text) = value_as_string(values[idx + 2].clone())
        {
            let expanded = resolve_script_pattern_patch(address, &pattern_text, &bytes)
                .map_err(mlua::Error::runtime)?;
            out.extend(expanded);
            idx += 3;
            continue;
        }

        out.push(ScriptPatch { address, bytes });
        idx += 2;
    }
    Ok(out)
}

fn parse_script_patch_table(table: Table) -> mlua::Result<Vec<ScriptPatch>> {
    if table.contains_key("address")? {
        return parse_single_patch_entry(table);
    }

    let mut out = Vec::new();
    for value in table.sequence_values::<Value>() {
        let entry = match value? {
            Value::Table(entry) => entry,
            _ => {
                return Err(mlua::Error::runtime(
                    "Engine.memory.patch table entries must be patch objects",
                ));
            }
        };
        out.extend(parse_single_patch_entry(entry)?);
    }
    Ok(out)
}

fn parse_single_patch_entry(entry: Table) -> mlua::Result<Vec<ScriptPatch>> {
    let address_value = entry.get::<Value>("address")?;
    let address = parse_lua_address(address_value).map_err(mlua::Error::runtime)?;
    let bytes =
        parse_hex_bytes_string(&entry.get::<String>("bytes")?).map_err(mlua::Error::runtime)?;
    if let Ok(pattern) = entry.get::<String>("pattern") {
        return resolve_script_pattern_patch(address, &pattern, &bytes)
            .map_err(mlua::Error::runtime);
    }
    Ok(vec![ScriptPatch { address, bytes }])
}

fn parse_lua_address(value: Value) -> std::result::Result<usize, String> {
    match value {
        Value::Integer(v) if v >= 0 => Ok(v as usize),
        Value::Number(v) if v.is_finite() && v >= 0.0 => Ok(v as usize),
        Value::String(v) => parse_address_string(v.to_str().map_err(|e| e.to_string())?.as_ref()),
        _ => Err("address must be a positive integer or hex string".to_string()),
    }
}

fn parse_address_string(raw: &str) -> std::result::Result<usize, String> {
    let value = raw.trim();
    if value.is_empty() {
        return Err("address string cannot be empty".to_string());
    }
    if let Some(hex) = value
        .strip_prefix("0x")
        .or_else(|| value.strip_prefix("0X"))
    {
        usize::from_str_radix(hex, 16).map_err(|err| format!("invalid hex address '{raw}': {err}"))
    } else {
        value
            .parse::<usize>()
            .map_err(|err| format!("invalid address '{raw}': {err}"))
    }
}

fn parse_hex_bytes_string(raw: &str) -> std::result::Result<Vec<u8>, String> {
    let trimmed = raw.trim();
    if trimmed.is_empty() {
        return Ok(Vec::new());
    }
    trimmed
        .split_whitespace()
        .map(|token| {
            if token.len() != 2 {
                return Err(format!("invalid byte '{token}': expected 2 hex digits"));
            }
            u8::from_str_radix(token, 16).map_err(|err| format!("invalid byte '{token}': {err}"))
        })
        .collect()
}

fn value_as_string(value: Value) -> mlua::Result<String> {
    match value {
        Value::String(v) => Ok(v.to_str()?.to_string()),
        _ => Err(mlua::Error::runtime("expected string argument")),
    }
}

fn resolve_script_pattern_patch(
    address: usize,
    pattern: &str,
    patch_bytes: &[u8],
) -> std::result::Result<Vec<ScriptPatch>, String> {
    let tokens: Vec<&str> = pattern.split_whitespace().collect();
    if tokens.is_empty() {
        return Err("pattern must not be empty".to_string());
    }

    let mut wildcard_blocks: Vec<(usize, usize)> = Vec::new();
    let mut idx = 0usize;
    while idx < tokens.len() {
        let token = tokens[idx];
        if token == "??" {
            let start = idx;
            while idx < tokens.len() && tokens[idx] == "??" {
                idx += 1;
            }
            wildcard_blocks.push((start, idx - start));
            continue;
        }
        if token.contains('?') {
            return Err(format!(
                "invalid wildcard token '{token}'; only full-byte wildcard '??' is allowed"
            ));
        }

        let expected = parse_hex_bytes_string(token)?
            .first()
            .copied()
            .ok_or_else(|| format!("invalid pattern byte '{token}'"))?;
        if !is_probably_readable(address + idx, 1) {
            return Err(format!(
                "pattern check failed at 0x{:X}: address is not readable",
                address + idx
            ));
        }
        let found = unsafe { ((address + idx) as *const u8).read() };
        if found != expected {
            return Err(format!(
                "pattern mismatch at 0x{:X}: expected {:02X}, found {:02X}",
                address + idx,
                expected,
                found
            ));
        }
        idx += 1;
    }

    if wildcard_blocks.is_empty() {
        return Err("pattern has no wildcard blocks to patch".to_string());
    }

    let wildcard_total: usize = wildcard_blocks.iter().map(|(_, len)| *len).sum();
    if patch_bytes.len() != wildcard_total {
        return Err(format!(
            "pattern wildcard bytes mismatch: expected {} replacement bytes, got {}",
            wildcard_total,
            patch_bytes.len()
        ));
    }

    let mut consumed = 0usize;
    let mut resolved = Vec::new();
    for (start, len) in wildcard_blocks {
        let end = consumed + len;
        resolved.push(ScriptPatch {
            address: address + start,
            bytes: patch_bytes[consumed..end].to_vec(),
        });
        consumed = end;
    }
    Ok(resolved)
}

#[cfg(unix)]
fn is_probably_readable(address: usize, length: usize) -> bool {
    is_probably_accessible_unix(address, length, libc::PROT_READ)
}

#[cfg(unix)]
fn is_probably_writable(address: usize, length: usize) -> bool {
    is_probably_accessible_unix(address, length, libc::PROT_WRITE)
}

#[cfg(unix)]
fn is_probably_accessible_unix(address: usize, length: usize, required_flags: i32) -> bool {
    if length == 0 {
        return true;
    }

    let content = match std::fs::read_to_string("/proc/self/maps") {
        Ok(content) => content,
        Err(_) => return false,
    };

    let end = match address.checked_add(length) {
        Some(end) => end,
        None => return false,
    };

    for line in content.lines() {
        let mut parts = line.split_whitespace();
        let range = match parts.next() {
            Some(value) => value,
            None => continue,
        };
        let perms = match parts.next() {
            Some(value) => value,
            None => continue,
        };
        let Some((start_raw, end_raw)) = range.split_once('-') else {
            continue;
        };
        let Ok(start) = usize::from_str_radix(start_raw, 16) else {
            continue;
        };
        let Ok(region_end) = usize::from_str_radix(end_raw, 16) else {
            continue;
        };
        let read_ok = perms.as_bytes().first().copied() == Some(b'r');
        let write_ok = perms.as_bytes().get(1).copied() == Some(b'w');
        let access_ok = (required_flags & libc::PROT_READ == 0 || read_ok)
            && (required_flags & libc::PROT_WRITE == 0 || write_ok);

        if access_ok && address >= start && end <= region_end {
            return true;
        }
    }
    false
}

// #[cfg(not(unix))]
// fn is_probably_readable(address: usize, length: usize) -> bool {
//     address.checked_add(length).is_some()
// }
//
// #[cfg(not(unix))]
// fn is_probably_writable(address: usize, length: usize) -> bool {
//     address.checked_add(length).is_some()
// }

fn install_engine_dispatch_api(
    lua: &Lua,
    engine_table: &Table,
    load_order: &[usize],
    mod_entries: &[(ModMeta, PathBuf)],
) -> Result<()> {
    let dispatch_targets: Vec<(String, HashSet<String>)> = load_order
        .iter()
        .map(|index| {
            let meta = &mod_entries[*index].0;
            (
                meta.id.clone(),
                meta.events.iter().cloned().collect::<HashSet<String>>(),
            )
        })
        .collect();

    let dispatch_fn = lua.create_function(move |lua, mut args: mlua::MultiValue| {
        let event_name = match args.pop_front() {
            Some(Value::String(name)) => match name.to_str() {
                Ok(value) => value.to_owned(),
                Err(_) => return Ok(()),
            },
            _ => return Ok(()),
        };

        let engine_table: Table = match lua.globals().get("Engine") {
            Ok(table) => table,
            Err(_) => return Ok(()),
        };

        for (mod_id, events) in &dispatch_targets {
            if !events.contains(event_name.as_str()) {
                continue;
            }

            let mod_table = match engine_table.get::<Table>(mod_id.as_str()) {
                Ok(table) => table,
                Err(_) => continue,
            };
            let handler = match mod_table.get::<Function>(event_name.as_str()) {
                Ok(function) => function,
                Err(_) => continue,
            };
            let _ = handler.call::<()>(args.clone());
        }

        Ok(())
    })?;

    engine_table.set("dispatch", dispatch_fn)?;
    Ok(())
}

/// Installs an `Engine.require(path)` function scoped to the currently loading mod.
fn register_engine_require(
    lua: &Lua,
    mod_path: PathBuf,
    required_files: Arc<Mutex<HashSet<PathBuf>>>,
) -> Result<()> {
    let globals = lua.globals();
    let engine_table: Table = globals.get("Engine")?;
    let require_mod_path = mod_path.clone();
    let require_fn = lua.create_function(move |lua, requested_path: String| {
        let trimmed = requested_path.trim();
        if trimmed.is_empty() {
            return Err(mlua::Error::runtime(
                "Engine.require file name cannot be empty",
            ));
        }

        let mod_root = require_mod_path.canonicalize().map_err(|err| {
            mlua::Error::runtime(format!("failed to resolve mod directory: {err}"))
        })?;
        let file_path = resolve_required_file_path(&mod_root, trimmed)?;
        modloader_trace!("Engine.require resolving '{}' to {:?}", trimmed, file_path);

        if !file_path.starts_with(&mod_root) {
            return Err(mlua::Error::runtime(format!(
                "Engine.require cannot access file outside mod directory: {trimmed}"
            )));
        }

        if let Ok(mut guard) = required_files.lock() {
            guard.insert(normalize_path(&file_path));
        }

        let script = std::fs::read_to_string(&file_path).map_err(|err| {
            mlua::Error::runtime(format!(
                "failed to read required file {:?}: {err}",
                file_path
            ))
        })?;

        let env = lua.create_table()?;
        let env_mt = lua.create_table()?;
        env_mt.set("__index", lua.globals())?;
        env.set_metatable(Some(env_mt))?;
        let chunk_result: mlua::Value = lua
            .load(&script)
            .set_name(file_path.to_string_lossy().as_ref())
            .set_environment(env.clone())
            .eval()?;

        let mut exports = lua.create_table()?;
        let mut seen_keys: HashSet<String> = HashSet::new();

        for pair in env.pairs::<mlua::Value, mlua::Value>() {
            let (key, value) = pair?;

            if matches!(value, mlua::Value::Nil) {
                continue;
            }

            let key_str = match key {
                mlua::Value::String(s) => s.to_str()?.to_owned(),
                _ => continue,
            };

            exports.set(key_str.clone(), value)?;
            seen_keys.insert(key_str);
        }

        if !matches!(chunk_result, mlua::Value::Nil) {
            let rel_path = file_path.strip_prefix(&mod_root).map_err(|err| {
                mlua::Error::runtime(format!("failed to build export path for {trimmed}: {err}"))
            })?;
            insert_named_return_value(lua, &mut exports, rel_path, chunk_result, &mut seen_keys)?;
        }

        Ok(exports)
    })?;

    engine_table.set("require", require_fn)?;
    modloader_trace!("Engine.require installed for mod root {:?}", mod_path);
    Ok(())
}

/// Resolves a relative require path into a canonical `.luau` script path.
fn resolve_required_file_path(mod_root: &Path, requested_path: &str) -> mlua::Result<PathBuf> {
    let relative = Path::new(requested_path);
    if relative.is_absolute() {
        return Err(mlua::Error::runtime(format!(
            "Engine.require expects a relative path, got absolute path: {requested_path}"
        )));
    }

    let joined = mod_root.join(relative);
    if joined.exists() {
        return joined.canonicalize().map_err(|err| {
            mlua::Error::runtime(format!(
                "failed to resolve required file {}: {err}",
                joined.display()
            ))
        });
    }

    let with_ext = joined.with_extension("luau");
    if with_ext.exists() {
        return with_ext.canonicalize().map_err(|err| {
            mlua::Error::runtime(format!(
                "failed to resolve required file {}: {err}",
                with_ext.display()
            ))
        });
    }

    Err(mlua::Error::runtime(format!(
        "required file not found for path: {requested_path}"
    )))
}

fn insert_named_return_value(
    lua: &Lua,
    exports: &mut Table,
    rel_path: &Path,
    value: mlua::Value,
    seen_keys: &mut HashSet<String>,
) -> mlua::Result<()> {
    let mut segments: Vec<String> = rel_path
        .iter()
        .map(|part| sanitize_identifier(part.to_string_lossy().as_ref()))
        .collect();
    if let Some(last) = segments.last_mut()
        && let Some(stripped) = last.strip_suffix(".luau")
    {
        *last = stripped.to_string();
    }
    if segments.is_empty() || segments.iter().any(|segment| segment.is_empty()) {
        return Err(mlua::Error::runtime(
            "required file path produced invalid empty export field name",
        ));
    }

    let mut cursor = exports.clone();
    for segment in &segments[0..segments.len().saturating_sub(1)] {
        if cursor.contains_key(segment.as_str())? {
            match cursor.get::<mlua::Value>(segment.as_str())? {
                mlua::Value::Table(existing_table) => cursor = existing_table,
                _ => {
                    return Err(mlua::Error::runtime(format!(
                        "cannot place required return object at '{}': field already exists",
                        segment
                    )));
                }
            }
        } else {
            let next = lua.create_table()?;
            cursor.set(segment.as_str(), next.clone())?;
            cursor = next;
        }
    }

    let final_field = segments
        .last()
        .expect("segments already checked to be non-empty");
    if seen_keys.contains(final_field) || cursor.contains_key(final_field.as_str())? {
        return Err(mlua::Error::runtime(format!(
            "cannot export required return object: field '{}' is already occupied",
            final_field
        )));
    }
    cursor.set(final_field.as_str(), value)?;
    seen_keys.insert(final_field.clone());
    Ok(())
}

/// Sanitizes path segments for nested table export field names.
fn sanitize_identifier(name: &str) -> String {
    name.replace(' ', "_")
}

/// Attempts to canonicalize a path; falls back to original path on failure.
fn normalize_path(path: &Path) -> PathBuf {
    path.canonicalize().unwrap_or_else(|_| path.to_path_buf())
}

/// Hashes file bytes for content-based hot-reload detection.
fn hash_file(path: &Path) -> Option<Hash> {
    match std::fs::read(path) {
        Ok(data) => Some(blake3::hash(&data)),
        Err(_) => None,
    }
}

/// Ensures an addon path is contained within the configured addons base.
fn is_safe_path(base: &Path, child: &Path) -> bool {
    match (base.canonicalize(), child.canonicalize()) {
        (Ok(b), Ok(c)) => c.starts_with(&b),
        _ => false,
    }
}
