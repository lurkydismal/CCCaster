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
use std::io;
use std::io::ErrorKind;
use std::os::fd::RawFd;
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, Ordering};
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
static SHUTDOWN_GUARD: Lazy<Mutex<Option<ShutdownGuard>>> = Lazy::new(|| Mutex::new(None));
static PARENT_DIED_SIGNALLED: AtomicBool = AtomicBool::new(false);

struct ShutdownGuard {
    pid: libc::pid_t,
    notify_fd: RawFd,
}

unsafe extern "C" fn parent_died_handler(_signal: libc::c_int) {
    PARENT_DIED_SIGNALLED.store(true, Ordering::SeqCst);
}

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
    start_shutdown_guard(&lua, &loaded_mods)?;

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
                call_quit_callbacks(&lua, &loaded_mods)?;
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

fn start_shutdown_guard(lua: &Lua, loaded_mods: &[LoadedMod]) -> Result<()> {
    let mut fds = [0; 2];
    if unsafe { libc::pipe(fds.as_mut_ptr()) } != 0 {
        return Err(anyhow!(
            "failed to create shutdown guard pipe: {}",
            io::Error::last_os_error()
        ));
    }

    let fork_pid = unsafe { libc::fork() };
    if fork_pid < 0 {
        unsafe {
            libc::close(fds[0]);
            libc::close(fds[1]);
        }
        return Err(anyhow!(
            "failed to fork shutdown guard process: {}",
            io::Error::last_os_error()
        ));
    }

    if fork_pid == 0 {
        unsafe {
            libc::close(fds[1]);
        }
        run_shutdown_guard_child(fds[0], lua, loaded_mods);
    }

    unsafe {
        libc::close(fds[0]);
    }
    let mut guard = SHUTDOWN_GUARD
        .lock()
        .map_err(|_| anyhow!("shutdown guard mutex poisoned"))?;
    *guard = Some(ShutdownGuard {
        pid: fork_pid,
        notify_fd: fds[1],
    });
    Ok(())
}

fn run_shutdown_guard_child(read_fd: RawFd, lua: &Lua, loaded_mods: &[LoadedMod]) -> ! {
    let prctl_result = unsafe { libc::prctl(libc::PR_SET_PDEATHSIG, libc::SIGUSR1) };
    if prctl_result != 0 {
        modloader_error!(
            "Shutdown guard failed to set parent-death signal: {}",
            io::Error::last_os_error()
        );
        unsafe { libc::_exit(1) }
    }

    let mut sig_action: libc::sigaction = unsafe { std::mem::zeroed() };
    sig_action.sa_flags = 0;
    sig_action.sa_sigaction = parent_died_handler as *const () as usize;
    unsafe {
        libc::sigemptyset(&mut sig_action.sa_mask);
        if libc::sigaction(libc::SIGUSR1, &sig_action, std::ptr::null_mut()) != 0 {
            modloader_error!(
                "Shutdown guard failed to install signal handler: {}",
                io::Error::last_os_error()
            );
            libc::_exit(1);
        }
    }

    if unsafe { libc::getppid() } == 1 {
        PARENT_DIED_SIGNALLED.store(true, Ordering::SeqCst);
    }

    let mut buf = [0u8; 1];
    loop {
        if PARENT_DIED_SIGNALLED.load(Ordering::SeqCst) {
            modloader_warning!("Parent process died; invoking quit callbacks from guard process");
            let _ = call_quit_callbacks(lua, loaded_mods);
            unsafe { libc::_exit(0) }
        }

        let read_result = unsafe { libc::read(read_fd, buf.as_mut_ptr().cast(), 1) };
        if read_result == 0 {
            unsafe { libc::_exit(0) }
        }
        if read_result > 0 {
            unsafe { libc::_exit(0) }
        }

        let err = io::Error::last_os_error();
        if err.kind() == ErrorKind::Interrupted {
            continue;
        }
        modloader_error!("Shutdown guard read failed: {}", err);
        unsafe { libc::_exit(1) }
    }
}

fn stop_shutdown_guard() {
    let guard = match SHUTDOWN_GUARD.lock() {
        Ok(mut lock) => lock.take(),
        Err(_) => {
            modloader_error!("Failed to lock shutdown guard state");
            None
        }
    };
    if let Some(guard) = guard {
        let _ = unsafe { libc::write(guard.notify_fd, [1u8; 1].as_ptr().cast(), 1) };
        unsafe {
            libc::close(guard.notify_fd);
        }
        let mut status = 0;
        let _ = unsafe { libc::waitpid(guard.pid, &mut status, 0) };
    }
}

pub fn shutdown_before_unload() {
    stop_shutdown_guard();
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
