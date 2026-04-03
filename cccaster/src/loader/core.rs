/// Core modloader lifecycle, discovery, load-order resolution, and hot-reload runtime.
use crate::api::with_suspend_override;
use crate::hook::{HookDecision, HookRegisterOverrides};
use crate::patch::{
    OwnedPatchSpan, PatchEntry, PatchSpan, Patches, ResolvedPatch, ensure_no_overlap,
    resolve_patch_entries, spans_for_patches,
};
use crate::types::{Dependency, ModMeta, RawModInfo};
use crate::{
    AppError, modloader_debug, modloader_error, modloader_info, modloader_trace, modloader_warning,
    runtime_args,
};
use anyhow::{Context, Result, anyhow};
use blake3::Hash;
use mlua::{Function, Lua, Table, Value};
use notify::{Event, EventKind, RecommendedWatcher, RecursiveMode, Watcher};
use once_cell::sync::Lazy;
use petgraph::algo::toposort;
use petgraph::graph::Graph;
use semver::{Version, VersionReq};
use serde_json::Value as JsonValue;
use std::collections::{HashMap, HashSet};
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant};
use tokio::fs;
use tokio::sync::oneshot;
use tree_sitter::Parser;

use super::engine_fs::{install_engine_env_api, register_engine_fs_local};
use super::engine_memory::{
    ENGINE_LOG_MOD_ID_KEY, install_engine_assert_api, install_engine_log_api,
    install_engine_memory_api,
};
use super::engine_require_dispatch::{install_engine_dispatch_api, register_engine_require};
use super::overlay_vfs::{init_overlay_registry, mount_process_local_overlay, register_mod_assets};
use super::path_utils::{hash_file, is_safe_path, normalize_path};

type StartupSignal = Arc<Mutex<Option<oneshot::Sender<Result<()>>>>>;
type ShutdownSender = std::sync::mpsc::Sender<ControlMessage>;

enum ControlMessage {
    Shutdown,
    RegisterEngineVariable {
        name: String,
        value: JsonValue,
    },
    DispatchEngineEvent {
        event_name: String,
        arg_values: Vec<String>,
        arg_types: Vec<String>,
    },
    DispatchHookEvent {
        event_name: String,
        payload_json: String,
        response_tx: std::sync::mpsc::Sender<HookDecision>,
    },
}

static HOT_RELOAD_THREAD: Lazy<Mutex<Option<JoinHandle<()>>>> = Lazy::new(|| Mutex::new(None));
static SHUTDOWN_SIGNAL: Lazy<Mutex<Option<ShutdownSender>>> = Lazy::new(|| Mutex::new(None));
static PENDING_UNLOADS: Lazy<Mutex<HashSet<String>>> = Lazy::new(|| Mutex::new(HashSet::new()));
static PENDING_ENGINE_VARIABLES: Lazy<Mutex<HashMap<String, JsonValue>>> =
    Lazy::new(|| Mutex::new(HashMap::new()));

pub fn register_engine_variable(name: String, value: JsonValue) -> Result<()> {
    modloader_trace!(
        "register_engine_variable called: raw_name='{}' value={}",
        name,
        value
    );
    let trimmed_name = name.trim();
    if trimmed_name.is_empty() {
        modloader_error!("register_engine_variable rejected empty name");
        return Err(anyhow!("Engine variable name cannot be empty"));
    }

    let key = trimmed_name.to_owned();
    {
        let mut pending = PENDING_ENGINE_VARIABLES
            .lock()
            .map_err(|_| anyhow!("engine variable registry mutex poisoned"))?;
        pending.insert(key.clone(), value.clone());
        modloader_debug!(
            "register_engine_variable staged '{}' (pending count={})",
            key,
            pending.len()
        );
    }

    let tx = SHUTDOWN_SIGNAL
        .lock()
        .map_err(|_| anyhow!("shutdown signal mutex poisoned"))?
        .as_ref()
        .cloned();

    if let Some(tx) = tx
        && let Err(err) = tx.send(ControlMessage::RegisterEngineVariable {
            name: key.clone(),
            value,
        })
    {
        modloader_warning!(
            "Failed to send live Engine variable update for '{}': {}",
            key,
            err
        );
    }
    modloader_info!("register_engine_variable accepted '{}'", key);

    Ok(())
}

pub fn dispatch_engine_event(
    event_name: String,
    arg_values: Vec<String>,
    arg_types: Vec<String>,
) -> Result<()> {
    modloader_trace!(
        "dispatch_engine_event called: event='{}', values={}, types={}",
        event_name,
        arg_values.len(),
        arg_types.len()
    );
    let trimmed_name = event_name.trim();
    if trimmed_name.is_empty() {
        modloader_error!("dispatch_engine_event rejected empty event name");
        return Err(anyhow!("Engine event name cannot be empty"));
    }
    if arg_values.len() != arg_types.len() {
        modloader_error!(
            "dispatch_engine_event rejected mismatched args for '{}': {} values vs {} types",
            trimmed_name,
            arg_values.len(),
            arg_types.len()
        );
        return Err(anyhow!(
            "Engine event argument value/type count mismatch: {} values vs {} types",
            arg_values.len(),
            arg_types.len()
        ));
    }

    let tx = SHUTDOWN_SIGNAL
        .lock()
        .map_err(|_| anyhow!("shutdown signal mutex poisoned"))?
        .as_ref()
        .cloned()
        .ok_or_else(|| {
            modloader_error!(
                "dispatch_engine_event called before runtime initialization for '{}'",
                trimmed_name
            );
            anyhow!("modloader runtime is not initialized")
        })?;

    modloader_debug!(
        "Sending runtime Engine event '{}' with payload {:?} and types {:?}",
        trimmed_name,
        arg_values,
        arg_types
    );
    tx.send(ControlMessage::DispatchEngineEvent {
        event_name: trimmed_name.to_owned(),
        arg_values,
        arg_types,
    })
    .map_err(|err| anyhow!("failed to send Engine event dispatch request: {}", err))
}

pub fn dispatch_hook_event_sync(event_name: String, payload_json: String) -> Result<HookDecision> {
    modloader_trace!(
        "dispatch_hook_event_sync called: event='{}', payload_len={}",
        event_name,
        payload_json.len()
    );
    let tx = SHUTDOWN_SIGNAL
        .lock()
        .map_err(|_| anyhow!("shutdown signal mutex poisoned"))?
        .as_ref()
        .cloned()
        .ok_or_else(|| {
            modloader_error!(
                "dispatch_hook_event_sync called before runtime initialization for '{}'",
                event_name
            );
            anyhow!("modloader runtime is not initialized")
        })?;
    let (response_tx, response_rx) = std::sync::mpsc::channel();

    tx.send(ControlMessage::DispatchHookEvent {
        event_name,
        payload_json,
        response_tx,
    })
    .map_err(|err| anyhow!("failed to send hook dispatch request: {}", err))?;
    modloader_trace!("dispatch_hook_event_sync sent event; waiting for hook response");

    response_rx
        .recv_timeout(Duration::from_millis(50))
        .map_err(|err| anyhow!("failed waiting for hook dispatch response: {}", err))
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

#[derive(Default)]
struct PendingReload {
    changed_paths: Vec<PathBuf>,
    patch_changed: bool,
    script_changed: bool,
    asset_changed: bool,
}

/// Scans the addons directory, initializes mods, and starts hot-reload in a background thread.
pub async fn load_mods_from_addons() -> Result<()> {
    let modloader_exe = std::env::current_exe().map_err(|err| {
        AppError::Message(format!("Failed to resolve current modloader path: {err}"))
    })?;
    let modloader_root = modloader_exe.parent().map(PathBuf::from).ok_or_else(|| {
        AppError::Message(format!(
            "Modloader path has no parent directory: {}",
            modloader_exe.display()
        ))
    })?;

    ensure_root_archive_available(&modloader_root, &["cccaster.tar.zstd"])?;
    ensure_archived_directory_available(
        &modloader_root,
        "converts",
        &["converts.tar.zstd", "cccaster.tar.zstd"],
    )?;
    init_overlay_registry(&modloader_root)?;
    mount_process_local_overlay()?;

    modloader_info!("load_mods_from_addons: creating startup and control channels");
    let (ready_tx, ready_rx) = oneshot::channel::<Result<()>>();
    let (control_tx, control_rx) = std::sync::mpsc::channel::<ControlMessage>();
    let ready_signal = Arc::new(Mutex::new(Some(ready_tx)));
    let thread_signal = ready_signal.clone();

    let hot_reload_thread = thread::Builder::new()
        .name("cccaster-hot-reload".to_string())
        .spawn(move || {
            modloader_debug!("hot-reload thread booting runtime");
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
        modloader_trace!("load_mods_from_addons: stored shutdown signal sender");
    }
    {
        let mut thread_guard = HOT_RELOAD_THREAD
            .lock()
            .map_err(|_| anyhow!("hot-reload thread mutex poisoned"))?;
        *thread_guard = Some(hot_reload_thread);
        modloader_trace!("load_mods_from_addons: stored hot-reload thread handle");
    }

    modloader_debug!("load_mods_from_addons: waiting for startup signal");
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
    modloader_trace!(
        "load_mods_from_addons_async args snapshot: play={} dry_run={} safe_mode={} timings={}",
        args.play,
        args.dry_run,
        args.safe_mode,
        args.timings
    );
    let startup_started = Instant::now();
    // FIX: Somehow resolve windows path here or in launcher
    // let addons_dir = PathBuf::from(args.addons_dir.as_deref().unwrap_or("addons"));
    // let addons_dir_path = addons_dir.as_path();
    // TODO: Accept launcher root and use here
    let arg0 = std::env::args()
        .next()
        .ok_or_else(|| AppError::Message("Missing argv[0]\n".to_string()))?;

    let mut modloader_exe = PathBuf::from(arg0);

    // If it's not absolute, resolve it against current working dir
    if modloader_exe.is_relative() {
        let cwd = std::env::current_dir()
            .map_err(|_| AppError::Message("Failed to resolve current directory\n".to_string()))?;

        modloader_exe = cwd.join(modloader_exe);
    }

    modloader_debug!(
        "Resolved current modloader path: {}",
        modloader_exe.display()
    );

    let modloader_root = modloader_exe
        .parent()
        .ok_or_else(|| AppError::Message("Invalid executable path\n".to_string()))?;

    let addons_dir = modloader_root.join("addons");
    ensure_archived_directory_available(
        modloader_root,
        "addons",
        &["addons.tar.zstd", "cccaster.tar.zstd"],
    )?;
    let addons_dir_path = addons_dir.as_path();
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

    if !addons_dir_path.exists() {
        modloader_warning!(
            "addons directory does not exist at {:?}; starting without addons",
            addons_dir_path
        );
    }

    let mut dir = match fs::read_dir(addons_dir_path).await {
        Ok(dir) => dir,
        Err(err) if err.kind() == std::io::ErrorKind::NotFound => {
            modloader_warning!(
                "addons directory missing at {:?}; continuing with zero discovered addons",
                addons_dir_path
            );
            send_startup_signal(&ready_signal, Ok(()));
            return Ok(());
        }
        Err(err) => {
            return Err(err).with_context(|| {
                format!("unable to open addons directory at {:?}", addons_dir_path)
            });
        }
    };
    while let Some(entry) = dir
        .next_entry()
        .await
        .with_context(|| format!("failed while iterating entries in {:?}", addons_dir_path))?
    {
        let path = entry.path();
        modloader_trace!("Inspecting addon entry at {:?}", path);
        if path.is_dir() || is_tar_zstd_archive(&path) {
            if !is_safe_path(addons_dir_path, &path) {
                modloader_warning!("Skipping unsafe directory path: {:?}", path);
                continue;
            }
            let (mod_id, mod_root) = if path.is_dir() {
                let Some(id) = derive_mod_id(&path) else {
                    modloader_warning!(
                        "Skipping addon path with invalid directory name: {:?}",
                        path
                    );
                    continue;
                };
                (id, path.clone())
            } else {
                let Some((id, extracted_root)) = prepare_archived_mod_dir(modloader_root, &path)?
                else {
                    continue;
                };
                (id, extracted_root)
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

            let info_path = mod_root.join("info.json");
            let info_data = fs::read(&info_path).await;
            if let Err(err) = info_data.as_ref() {
                modloader_error!(
                    "Failed to read mod manifest {:?} for {:?}: {}. Skipping this addon.",
                    info_path,
                    mod_root,
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
                        mod_root,
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
                    assets_ignore: raw.assets.ignore.clone(),
                },
                mod_root.clone(),
            ));
            modloader_debug!(
                "Registered addon candidate '{}' from {:?}",
                mod_id,
                mod_root
            );
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
    install_engine_assert_api(&lua, &engine_table)?;
    install_engine_memory_api(&lua, &engine_table)?;
    install_engine_dispatch_api(&lua, &engine_table, load_order, mod_entries)?;
    install_engine_env_api(&lua, &engine_table)?;
    install_engine_unload_api(&lua, &engine_table)?;
    apply_registered_engine_variables(&lua, &engine_table)?;
    globals.set("Engine", engine_table)?;

    let mut loaded_mods: Vec<LoadedMod> = Vec::new();
    for &mod_index in load_order {
        let (meta, path) = &mod_entries[mod_index];
        modloader_info!("Loading mod '{}' from {:?}", meta.id, path);
        register_mod_assets(path, &meta.assets_ignore)?;
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
        process_pending_unloads(&lua, &mut watcher, &mut loaded_mods);

        if let Ok(message) = control_rx.try_recv() {
            match message {
                ControlMessage::Shutdown => {
                    modloader_info!("Shutdown signal received; invoking quit callbacks");
                    call_quit_callbacks(&lua, &loaded_mods)?;
                    return Ok(());
                }
                ControlMessage::RegisterEngineVariable { name, value } => {
                    let engine_table: Table = lua.globals().get("Engine")?;
                    set_engine_variable(&lua, &engine_table, &name, &value)?;
                    modloader_info!("Applied runtime Engine variable override '{}'", name);
                }
                ControlMessage::DispatchEngineEvent {
                    event_name,
                    arg_values,
                    arg_types,
                } => {
                    dispatch_runtime_engine_event(&lua, &event_name, &arg_values, &arg_types)?;
                    modloader_info!(
                        "Dispatched runtime Engine event '{}' with {} args",
                        event_name,
                        arg_values.len()
                    );
                }
                ControlMessage::DispatchHookEvent {
                    event_name,
                    payload_json,
                    response_tx,
                } => {
                    let result = dispatch_runtime_hook_event(&lua, &event_name, &payload_json)
                        .map_err(|err| {
                            anyhow!("hook event dispatch failed for '{}': {}", event_name, err)
                        });
                    let _ = response_tx.send(result.unwrap_or_default());
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

        let mut mods_to_reload: HashMap<usize, PendingReload> = HashMap::new();
        for path in &event.paths {
            if let Some(changed_index) = detect_changed_mod(&mut loaded_mods, path) {
                let normalized = normalize_path(path);
                let (kind, is_patch_change, is_script_change, is_asset_change) =
                    classify_hot_reload_path(&loaded_mods[changed_index].path, &normalized);
                if !is_patch_change && !is_script_change && !is_asset_change {
                    continue;
                }
                let pending = mods_to_reload.entry(changed_index).or_default();
                pending.changed_paths.push(normalized);
                pending.patch_changed |= is_patch_change;
                pending.script_changed |= is_script_change;
                pending.asset_changed |= is_asset_change;
                modloader_trace!(
                    "Queued {} hot-reload update for mod {}",
                    kind,
                    loaded_mods[changed_index].meta.id
                );
            }
        }

        for (mod_index, pending) in mods_to_reload {
            let meta = loaded_mods[mod_index].meta.clone();
            let path = loaded_mods[mod_index].path.clone();
            let args = runtime_args();

            let reload_result = if pending.script_changed {
                hot_reload_scripts_only(&lua, &mut loaded_mods, mod_index).await
            } else {
                if pending.patch_changed {
                    let occupied = collect_occupied_spans(&loaded_mods, Some(mod_index));
                    let (patches, spans, patch_count) = hot_reload_patches_only(
                        &loaded_mods[mod_index].meta,
                        &loaded_mods[mod_index].path,
                        &occupied,
                        args.dump_patches,
                    )
                    .await?;
                    loaded_mods[mod_index]._patches = patches;
                    loaded_mods[mod_index].patch_spans = spans;
                    modloader_info!(
                        "Hot-reloaded patch.json for mod {} ({} patch entries)",
                        loaded_mods[mod_index].meta.id,
                        patch_count
                    );
                }
                if pending.asset_changed {
                    register_mod_assets(
                        &loaded_mods[mod_index].path,
                        &loaded_mods[mod_index].meta.assets_ignore,
                    )?;
                    modloader_info!(
                        "Hot-reloaded assets for mod {}",
                        loaded_mods[mod_index].meta.id
                    );
                }
                Ok(())
            };

            if let Err(err) = reload_result {
                modloader_error!("Hot-reload failed for mod {}: {}", meta.id, err);
            } else {
                modloader_info!("Hot-reloaded mod {} from {:?}", meta.id, path);
            }
        }

        process_pending_unloads(&lua, &mut watcher, &mut loaded_mods);
    }
}

fn dispatch_runtime_engine_event(
    lua: &Lua,
    event_name: &str,
    arg_values: &[String],
    arg_types: &[String],
) -> Result<()> {
    modloader_trace!(
        "dispatch_runtime_engine_event entering: event='{}', argc={}",
        event_name,
        arg_values.len()
    );
    let dispatch_fn: Function = lua.globals().get::<Table>("Engine")?.get("dispatch")?;
    let mut args = mlua::MultiValue::new();
    args.push_back(Value::String(lua.create_string(event_name)?));

    for (value, kind) in arg_values.iter().zip(arg_types.iter()) {
        modloader_trace!(
            "dispatch_runtime_engine_event converting arg type='{}' raw='{}'",
            kind,
            value
        );
        args.push_back(parse_dispatch_argument(lua, value, kind)?);
    }

    dispatch_fn.call::<()>(args)?;
    modloader_info!(
        "dispatch_runtime_engine_event completed for '{}' with {} args",
        event_name,
        arg_values.len()
    );
    Ok(())
}

fn dispatch_runtime_hook_event(
    lua: &Lua,
    event_name: &str,
    payload_json: &str,
) -> Result<HookDecision> {
    modloader_trace!(
        "dispatch_runtime_hook_event entering: event='{}', payload_len={}",
        event_name,
        payload_json.len()
    );
    let dispatch_fn: Function = lua.globals().get::<Table>("Engine")?.get("dispatch")?;
    let payload_json = serde_json::from_str::<JsonValue>(payload_json)
        .map_err(|err| anyhow!("invalid hook payload JSON: {}", err))?;
    let payload_value = json_value_to_lua(lua, &payload_json)?;
    let result: Value = dispatch_fn.call((event_name, payload_value))?;
    let parsed = parse_hook_register_overrides(&result)?;
    let run_trampoline = parse_hook_run_trampoline(&result)?;
    let trampoline_bytes = parse_hook_trampoline_bytes(&result)?;
    let decision = HookDecision {
        registers: parsed,
        run_trampoline,
        trampoline_bytes,
    };
    modloader_debug!(
        "dispatch_runtime_hook_event completed for '{}'; overrides_present={} run_trampoline={} trampoline_override={}",
        event_name,
        decision.registers.is_some(),
        decision.run_trampoline,
        decision.trampoline_bytes.is_some()
    );
    Ok(decision)
}

fn parse_hook_register_overrides(value: &Value) -> Result<Option<HookRegisterOverrides>> {
    let Value::Table(table) = value else {
        modloader_trace!("parse_hook_register_overrides: callback returned non-table");
        return Ok(None);
    };

    let regs = HookRegisterOverrides {
        eax: table.get::<Option<u32>>("eax")?,
        ebx: table.get::<Option<u32>>("ebx")?,
        ecx: table.get::<Option<u32>>("ecx")?,
        edx: table.get::<Option<u32>>("edx")?,
        esi: table.get::<Option<u32>>("esi")?,
        edi: table.get::<Option<u32>>("edi")?,
        ebp: table.get::<Option<u32>>("ebp")?,
        esp_at_pushad: table.get::<Option<u32>>("esp")?,
    };
    let overrides = [
        regs.eax,
        regs.ebx,
        regs.ecx,
        regs.edx,
        regs.esi,
        regs.edi,
        regs.ebp,
        regs.esp_at_pushad,
    ]
    .iter()
    .filter(|value| value.is_some())
    .count();

    if overrides == 0 {
        modloader_trace!(
            "parse_hook_register_overrides: table returned without register overrides"
        );
        return Ok(None);
    }

    modloader_debug!(
        "parse_hook_register_overrides accepted {} register overrides",
        overrides
    );
    Ok(Some(regs))
}

fn parse_hook_run_trampoline(value: &Value) -> Result<bool> {
    let Value::Table(table) = value else {
        return Ok(false);
    };
    Ok(table
        .get::<Option<bool>>("run_trampoline")?
        .unwrap_or(false))
}

fn parse_hook_trampoline_bytes(value: &Value) -> Result<Option<Vec<u8>>> {
    let Value::Table(table) = value else {
        return Ok(None);
    };
    let Some(raw) = table.get::<Option<String>>("trampoline")? else {
        return Ok(None);
    };
    let parsed = parse_hook_hex_bytes_string(&raw)?;
    if parsed.is_empty() {
        return Err(anyhow!("hook trampoline override cannot be empty"));
    }
    Ok(Some(parsed))
}

fn parse_hook_hex_bytes_string(raw: &str) -> Result<Vec<u8>> {
    let trimmed = raw.trim();
    if trimmed.is_empty() {
        return Ok(Vec::new());
    }
    trimmed
        .split_whitespace()
        .map(|token| {
            if token.len() != 2 {
                return Err(anyhow!(
                    "invalid trampoline byte '{}': expected 2 hex digits",
                    token
                ));
            }
            u8::from_str_radix(token, 16)
                .map_err(|err| anyhow!("invalid trampoline byte '{}': {}", token, err))
        })
        .collect()
}

fn parse_dispatch_argument(lua: &Lua, raw_value: &str, raw_type: &str) -> Result<Value> {
    let kind = raw_type.trim().to_ascii_lowercase();
    modloader_trace!(
        "parse_dispatch_argument: raw_type='{}' normalized='{}' raw_value='{}'",
        raw_type,
        kind,
        raw_value
    );
    match kind.as_str() {
        "string" => Ok(Value::String(lua.create_string(raw_value)?)),
        "integer" | "int" => {
            let parsed = raw_value
                .trim()
                .parse::<i32>()
                .map_err(|err| anyhow!("invalid integer argument '{}': {}", raw_value, err))?;
            Ok(Value::Integer(parsed))
        }
        "number" | "float" | "double" => {
            let parsed = raw_value
                .trim()
                .parse::<f64>()
                .map_err(|err| anyhow!("invalid number argument '{}': {}", raw_value, err))?;
            Ok(Value::Number(parsed))
        }
        "bool" | "boolean" => {
            let parsed = raw_value
                .trim()
                .parse::<bool>()
                .map_err(|err| anyhow!("invalid boolean argument '{}': {}", raw_value, err))?;
            Ok(Value::Boolean(parsed))
        }
        "nil" | "null" => Ok(Value::Nil),
        "json" => {
            let json = serde_json::from_str::<JsonValue>(raw_value)
                .map_err(|err| anyhow!("invalid JSON argument payload '{}': {}", raw_value, err))?;
            json_value_to_lua(lua, &json)
        }
        _ => Err(anyhow!(
            "unsupported argument type '{}' (supported: string, integer, number, boolean, nil, json)",
            raw_type
        )),
    }
}

pub fn shutdown_before_unload() {
    modloader_info!("shutdown_before_unload requested");
    let tx = match SHUTDOWN_SIGNAL.lock() {
        Ok(mut guard) => guard.take(),
        Err(_) => {
            modloader_error!("Failed to lock shutdown signal for unload");
            None
        }
    };
    if let Some(tx) = tx {
        if let Err(err) = tx.send(ControlMessage::Shutdown) {
            modloader_warning!("Failed to signal hot-reload shutdown: {}", err);
        } else {
            modloader_debug!("shutdown_before_unload sent shutdown signal");
        }
    } else {
        modloader_trace!("shutdown_before_unload found no active shutdown signal sender");
    }

    let thread_handle = match HOT_RELOAD_THREAD.lock() {
        Ok(mut guard) => guard.take(),
        Err(_) => {
            modloader_error!("Failed to lock hot-reload thread handle for unload");
            None
        }
    };
    if let Some(handle) = thread_handle {
        if let Err(_panic) = handle.join() {
            modloader_error!("Hot-reload thread panicked while shutting down");
        } else {
            modloader_info!("shutdown_before_unload completed");
        }
    } else {
        modloader_trace!("shutdown_before_unload had no hot-reload thread to join");
    }
}

fn send_startup_signal(ready_signal: &StartupSignal, result: Result<()>) {
    modloader_trace!("send_startup_signal invoked");
    if let Ok(mut guard) = ready_signal.lock()
        && let Some(sender) = guard.take()
    {
        modloader_trace!("send_startup_signal delivering startup result");
        let _ = sender.send(result);
    } else {
        modloader_warning!("send_startup_signal skipped because signal was already consumed");
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
        let mut watched = loaded.watched_hashes.contains_key(&normalized);
        if !watched && is_asset_path(&loaded.path, &normalized) {
            watched = true;
            loaded.watched_hashes.insert(normalized.clone(), None);
        }
        if !watched {
            continue;
        }

        let previous_hash = loaded
            .watched_hashes
            .get(&normalized)
            .copied()
            .unwrap_or(None);
        let current_hash = hash_file(&normalized);
        if previous_hash != current_hash {
            modloader_debug!(
                "Detected actual content change for mod {} in {:?}",
                loaded.meta.id,
                normalized
            );
            loaded
                .watched_hashes
                .insert(normalized.clone(), current_hash);
            return Some(idx);
        }
    }
    None
}

fn is_asset_path(mod_path: &Path, normalized_path: &Path) -> bool {
    normalized_path.starts_with(normalize_path(&mod_path.join("assets")))
        || normalized_path == normalize_path(&mod_path.join("assets.tar.zstd"))
}

fn classify_hot_reload_path(mod_path: &Path, changed: &Path) -> (&'static str, bool, bool, bool) {
    let patch_path = normalize_path(&mod_path.join("patch.json"));
    let main_path = normalize_path(&mod_path.join("main.luau"));
    let script_archive_path = normalize_path(&mod_path.join("script.tar.zstd"));
    let is_patch = changed == patch_path;
    let is_asset = is_asset_path(mod_path, changed);
    let is_script = !is_patch
        && !is_asset
        && (changed == main_path
            || changed == script_archive_path
            || changed
                .extension()
                .and_then(|ext| ext.to_str())
                .is_some_and(|ext| ext.eq_ignore_ascii_case("luau")));
    let kind = if is_patch {
        "patch"
    } else if is_asset {
        "asset"
    } else if is_script {
        "script"
    } else {
        "other"
    };
    (kind, is_patch, is_script, is_asset)
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

async fn hot_reload_scripts_only(
    lua: &Lua,
    loaded_mods: &mut [LoadedMod],
    mod_index: usize,
) -> Result<()> {
    let meta = loaded_mods[mod_index].meta.clone();
    let path = loaded_mods[mod_index].path.clone();
    let archive_scripts_root = prepare_script_archive_dir(&path)?;
    let Some(main_path) = resolve_script_path(&path, archive_scripts_root.as_deref(), "main.luau")
    else {
        modloader_warning!(
            "Skipping script-only hot-reload for mod {} because main.luau is missing",
            meta.id
        );
        return Ok(());
    };

    modloader_info!("Starting script-only hot-reload for mod {}", meta.id);
    let unload_payload = run_unload(lua, &meta.id)?;

    let required_files = Arc::new(Mutex::new(HashSet::new()));
    register_engine_require(
        lua,
        path.clone(),
        archive_scripts_root.clone(),
        required_files.clone(),
    )
    .with_context(|| format!("failed to register Engine.require for mod {}", meta.id))?;
    register_engine_fs_local(lua, path.clone())
        .with_context(|| format!("failed to register Engine.fs.local for mod {}", meta.id))?;

    let script = fs::read_to_string(&main_path).await.with_context(|| {
        format!(
            "failed to read main script {:?} for mod {}",
            main_path, meta.id
        )
    })?;
    let used_script_paths = collect_used_script_files(
        &meta.id,
        &path,
        archive_scripts_root.as_deref(),
        &main_path,
        &script,
    )
    .with_context(|| format!("failed to discover used Luau files for mod {}", meta.id))?;
    for used_path in &used_script_paths {
        let source = fs::read_to_string(used_path).await.with_context(|| {
            format!(
                "failed to read used script {:?} for mod {} during precheck",
                used_path, meta.id
            )
        })?;
        precheck_luau_chunk(lua, &meta.id, used_path, &source)
            .with_context(|| format!("Luau precheck failed for mod {}", meta.id))?;
    }
    if let Ok(mut guard) = required_files.lock() {
        for used_path in used_script_paths {
            guard.insert(normalize_path(&used_path));
        }
    }

    set_engine_log_mod_id(lua, Some(meta.id.as_str()))?;
    let eval_result = lua
        .load(&script)
        .set_name(main_path.to_string_lossy().as_ref())
        .eval()
        .with_context(|| {
            format!(
                "failed to evaluate Lua script {:?} for mod {}",
                main_path, meta.id
            )
        });
    set_engine_log_mod_id(lua, None)?;
    let returned: Table = eval_result?;

    let engine_table: Table = lua.globals().get("Engine")?;
    engine_table.set(meta.id.clone(), returned.clone())?;
    if let Ok(load) = returned.get::<Function>("load") {
        modloader_debug!("Calling load() for mod {}", meta.id);
        set_engine_log_mod_id(lua, Some(meta.id.as_str()))?;
        let load_result = if matches!(unload_payload, Value::Nil) {
            load.call::<()>(())
                .with_context(|| format!("load() failed for mod {}", meta.id))
        } else {
            load.call::<()>(unload_payload)
                .with_context(|| format!("load() failed for mod {}", meta.id))
        };
        set_engine_log_mod_id(lua, None)?;
        load_result?;
    }

    let mut watched_hashes = HashMap::new();
    for watched_path in build_watched_files(&path, &required_files) {
        watched_hashes.insert(watched_path.clone(), hash_file(&watched_path));
    }
    loaded_mods[mod_index].watched_hashes = watched_hashes;

    modloader_info!("Finished script-only hot-reload for mod {}", meta.id);
    Ok(())
}

async fn hot_reload_patches_only(
    meta: &ModMeta,
    mod_path: &Path,
    occupied_spans: &[OwnedPatchSpan<'_>],
    dump_patches: bool,
) -> Result<(Option<Patches>, Vec<PatchSpan>, usize)> {
    let patch_path = mod_path.join("patch.json");
    if !patch_path.exists() {
        return Ok((None, Vec::new(), 0));
    }

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
                patch.address(),
                patch.len()
            );
        }
    }
    Ok((
        Some(Patches::new(&resolved_entries, true)),
        spans,
        resolved_entries.len(),
    ))
}

fn set_engine_log_mod_id(lua: &Lua, mod_id: Option<&str>) -> Result<()> {
    match mod_id {
        Some(value) => lua
            .set_named_registry_value(ENGINE_LOG_MOD_ID_KEY, value)
            .context("failed to set Engine.log mod context")?,
        None => lua
            .unset_named_registry_value(ENGINE_LOG_MOD_ID_KEY)
            .context("failed to clear Engine.log mod context")?,
    }
    Ok(())
}

fn install_engine_unload_api(lua: &Lua, engine_table: &Table) -> Result<()> {
    let unload_fn = lua.create_function(|lua, ()| {
        let mod_id = lua
            .named_registry_value::<String>(ENGINE_LOG_MOD_ID_KEY)
            .ok()
            .map(|id| id.trim().to_owned())
            .filter(|id| !id.is_empty());

        let Some(mod_id) = mod_id else {
            return Err(mlua::Error::runtime(
                "Engine.unload must be called from a mod callback context",
            ));
        };

        if let Ok(mut guard) = PENDING_UNLOADS.lock() {
            guard.insert(mod_id);
            return Ok(true);
        }

        Err(mlua::Error::runtime(
            "Engine.unload failed: pending unload queue is unavailable",
        ))
    })?;

    engine_table.set("unload", unload_fn)?;
    Ok(())
}

fn drain_pending_unloads() -> HashSet<String> {
    if let Ok(mut guard) = PENDING_UNLOADS.lock() {
        return std::mem::take(&mut *guard);
    }
    HashSet::new()
}

fn process_pending_unloads(
    lua: &Lua,
    watcher: &mut RecommendedWatcher,
    loaded_mods: &mut Vec<LoadedMod>,
) {
    let to_unload = drain_pending_unloads();
    if to_unload.is_empty() {
        return;
    }

    let engine_table: Table = match lua.globals().get("Engine") {
        Ok(table) => table,
        Err(err) => {
            modloader_error!(
                "Failed to fetch Engine table for unload processing: {}",
                err
            );
            return;
        }
    };

    for mod_id in &to_unload {
        if let Some(index) = loaded_mods
            .iter()
            .position(|loaded| loaded.meta.id == *mod_id)
        {
            let unloaded = loaded_mods.remove(index);
            if let Err(err) = watcher.unwatch(&unloaded.path) {
                modloader_warning!(
                    "Failed to stop hot-reload watch for mod {} at {:?}: {}",
                    mod_id,
                    unloaded.path,
                    err
                );
            }
            if let Err(err) = engine_table.set(mod_id.as_str(), Value::Nil) {
                modloader_warning!(
                    "Failed to unregister Engine table entry for mod {}: {}",
                    mod_id,
                    err
                );
            }
            modloader_info!("Unloaded mod {} and removed it from hot-reload", mod_id);
            continue;
        }

        modloader_warning!(
            "Engine.unload requested for unknown/unloaded mod {}; ignoring",
            mod_id
        );
    }
}

fn apply_registered_engine_variables(lua: &Lua, engine_table: &Table) -> Result<()> {
    let pending = PENDING_ENGINE_VARIABLES
        .lock()
        .map_err(|_| anyhow!("engine variable registry mutex poisoned"))?;
    for (name, value) in pending.iter() {
        set_engine_variable(lua, engine_table, name, value)?;
        modloader_debug!(
            "Registered Engine['{}'] from external dynamic library value",
            name
        );
    }
    Ok(())
}

fn set_engine_variable(
    lua: &Lua,
    engine_table: &Table,
    name: &str,
    value: &JsonValue,
) -> Result<()> {
    let lua_value = json_value_to_lua(lua, value)?;
    let external_table = match engine_table.get::<Value>("external")? {
        Value::Nil => {
            let table = lua.create_table()?;
            engine_table.set("external", table.clone())?;
            table
        }
        Value::Table(table) => table,
        _ => {
            return Err(anyhow!(
                "Engine.external exists but is not a table; cannot register '{}'",
                name
            ));
        }
    };
    external_table.set(name, lua_value)?;
    Ok(())
}

fn json_value_to_lua(lua: &Lua, value: &JsonValue) -> Result<Value> {
    match value {
        JsonValue::Null => Ok(Value::Nil),
        JsonValue::Bool(boolean) => Ok(Value::Boolean(*boolean)),
        JsonValue::Number(number) => {
            if let Some(integer) = number.as_i64() {
                Ok(Value::Integer(integer.try_into().map_err(|_| {
                    anyhow!("integer out of range for target type")
                })?))
            } else if let Some(float) = number.as_f64() {
                Ok(Value::Number(float))
            } else {
                Err(anyhow!("unsupported numeric value in JSON payload"))
            }
        }
        JsonValue::String(text) => Ok(Value::String(lua.create_string(text)?)),
        JsonValue::Array(items) => {
            let table = lua.create_table()?;
            for (idx, item) in items.iter().enumerate() {
                table.set((idx + 1) as i64, json_value_to_lua(lua, item)?)?;
            }
            Ok(Value::Table(table))
        }
        JsonValue::Object(entries) => {
            let table = lua.create_table()?;
            for (key, entry) in entries {
                table.set(key.as_str(), json_value_to_lua(lua, entry)?)?;
            }
            Ok(Value::Table(table))
        }
    }
}

async fn run_mod_tests(lua: &Lua, mod_id: &str, mod_path: &Path, tests_path: &Path) -> Result<()> {
    modloader_info!("Running tests.luau for mod {}", mod_id);
    let script = fs::read_to_string(tests_path).await.with_context(|| {
        format!(
            "failed to read tests script {:?} for mod {}",
            tests_path, mod_id
        )
    })?;
    let used_script_paths = collect_used_script_files(mod_id, mod_path, None, tests_path, &script)
        .with_context(|| format!("failed to discover used Luau test files for mod {}", mod_id))?;
    for used_path in &used_script_paths {
        let source = fs::read_to_string(used_path).await.with_context(|| {
            format!(
                "failed to read used test script {:?} for mod {} during precheck",
                used_path, mod_id
            )
        })?;
        precheck_luau_chunk(lua, mod_id, used_path, &source)
            .with_context(|| format!("Luau test precheck failed for mod {}", mod_id))?;
    }

    register_engine_require(
        lua,
        mod_path.to_path_buf(),
        None,
        Arc::new(Mutex::new(HashSet::new())),
    )
    .with_context(|| {
        format!(
            "failed to register Engine.require for tests in mod {}",
            mod_id
        )
    })?;

    let engine_table: Table = lua.globals().get("Engine")?;
    with_mocked_test_apis(lua, &engine_table, || {
        set_engine_log_mod_id(lua, Some(mod_id))?;
        let run_result = (|| -> Result<()> {
            let returned: Table = lua
                .load(&script)
                .set_name(tests_path.to_string_lossy().as_ref())
                .eval()
                .with_context(|| {
                    format!(
                        "failed to evaluate tests script {:?} for mod {}",
                        tests_path, mod_id
                    )
                })?;
            let main: Function = returned.get("main").with_context(|| {
                format!(
                    "tests.luau for mod {} must return a table with main()",
                    mod_id
                )
            })?;
            let outcome = main
                .call::<bool>(())
                .with_context(|| format!("tests.luau main() failed for mod {}", mod_id))?;
            if !outcome {
                return Err(anyhow!(
                    "tests.luau main() returned false for mod {}",
                    mod_id
                ));
            }
            Ok(())
        })();
        set_engine_log_mod_id(lua, None)?;
        run_result
    })?;

    modloader_info!("tests.luau passed for mod {}", mod_id);
    Ok(())
}

fn with_mocked_test_apis<F>(lua: &Lua, engine_table: &Table, run: F) -> Result<()>
where
    F: FnOnce() -> Result<()>,
{
    let original_fs: Value = engine_table.get("fs").unwrap_or(Value::Nil);
    let original_memory: Value = engine_table.get("memory").unwrap_or(Value::Nil);
    let mocked = install_mock_test_apis(lua, engine_table);
    if let Err(err) = mocked {
        return Err(anyhow!(err.to_string()));
    }

    let result = run();
    if matches!(original_fs, Value::Nil) {
        engine_table.raw_remove("fs")?;
    } else {
        engine_table.set("fs", original_fs)?;
    }
    if matches!(original_memory, Value::Nil) {
        engine_table.raw_remove("memory")?;
    } else {
        engine_table.set("memory", original_memory)?;
    }
    result
}

fn install_mock_test_apis(lua: &Lua, engine_table: &Table) -> mlua::Result<()> {
    let fs_table = lua.create_table()?;
    let local_table = lua.create_table()?;
    let mode_table = lua.create_table()?;
    mode_table.set("write", "write")?;
    mode_table.set("append", "append")?;
    fs_table.set("mode", mode_table)?;

    let fs_entries = Arc::new(Mutex::new(HashMap::<String, String>::new()));
    let read_entries = fs_entries.clone();
    local_table.set(
        "read",
        lua.create_function(move |_, path: String| {
            let guard = read_entries
                .lock()
                .map_err(|_| mlua::Error::runtime("mock fs lock poisoned"))?;
            Ok(guard.get(path.trim()).cloned())
        })?,
    )?;
    let write_entries = fs_entries.clone();
    local_table.set(
        "write",
        lua.create_function(
            move |_, (path, content, mode): (String, String, Option<String>)| {
                let key = path.trim().to_owned();
                let mut guard = write_entries
                    .lock()
                    .map_err(|_| mlua::Error::runtime("mock fs lock poisoned"))?;
                let is_append = mode
                    .as_deref()
                    .map(str::trim)
                    .map(|value| {
                        value.eq_ignore_ascii_case("append") || value.eq_ignore_ascii_case("a")
                    })
                    .unwrap_or(false);
                if is_append {
                    guard.entry(key).or_default().push_str(&content);
                } else {
                    guard.insert(key, content);
                }
                Ok(true)
            },
        )?,
    )?;
    fs_table.set("local", local_table)?;

    let memory_table = lua.create_table()?;
    memory_table.set(
        "read",
        lua.create_function(|_, (_addr, _len): (Value, usize)| Ok(Value::Nil))?,
    )?;
    memory_table.set(
        "write",
        lua.create_function(|_, (_addr, _bytes): (Value, String)| Ok(true))?,
    )?;
    let patch_table = lua.create_table()?;
    patch_table.set(
        "make",
        lua.create_function(|_, _args: mlua::MultiValue| Ok(Value::Integer(1)))?,
    )?;
    patch_table.set("remove", lua.create_function(|_, _id: u32| Ok(true))?)?;
    memory_table.set("patch", patch_table)?;

    engine_table.set("fs", fs_table)?;
    engine_table.set("memory", memory_table)?;
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
        set_engine_log_mod_id(lua, Some(mod_id))?;
        let result = unload
            .call::<Value>(())
            .with_context(|| format!("unload() failed for mod {}", mod_id));
        set_engine_log_mod_id(lua, None)?;
        return result;
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
    let patch_path = path.join("patch.json");
    let has_patch_file = patch_path.exists();
    let archive_scripts_root = prepare_script_archive_dir(&path)?;
    let main_path = resolve_script_path(&path, archive_scripts_root.as_deref(), "main.luau");
    let has_main_script = main_path.is_some();
    let tests_path = resolve_script_path(&path, archive_scripts_root.as_deref(), "tests.luau");

    let assets_path = path.join("assets");
    let has_assets_dir = assets_path.exists() && assets_path.is_dir();
    let has_assets_archive = path.join("assets.tar.zstd").is_file();
    let has_scripts_archive = path.join("script.tar.zstd").is_file();
    if !has_patch_file
        && !has_main_script
        && !has_assets_dir
        && !has_assets_archive
        && !has_scripts_archive
    {
        return Err(anyhow!(
            "mod {} must provide at least one of main.luau, script.tar.zstd, patch.json, assets/, or assets.tar.zstd",
            meta.id
        ));
    }

    if let Some(tests_path) = tests_path.as_ref() {
        run_mod_tests(lua, &meta.id, &path, tests_path)
            .await
            .with_context(|| format!("tests.luau failed for mod {}", meta.id))?;
    }

    let required_files = Arc::new(Mutex::new(HashSet::new()));
    register_engine_require(
        lua,
        path.clone(),
        archive_scripts_root.clone(),
        required_files.clone(),
    )
    .with_context(|| format!("failed to register Engine.require for mod {}", meta.id))?;
    register_engine_fs_local(lua, path.clone())
        .with_context(|| format!("failed to register Engine.fs.local for mod {}", meta.id))?;

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
                    patch.address(),
                    patch.len()
                );
            }
        }
        modloader_info!(
            "Applying {} patch entries for mod {}",
            resolved_entries.len(),
            meta.id
        );
        (Some(Patches::new(&resolved_entries, false)), spans)
    } else {
        modloader_trace!("No patch.json present for mod {}", meta.id);
        (None, Vec::new())
    };

    let returned: Table = if let Some(main_path) = main_path.as_ref() {
        let script = fs::read_to_string(main_path).await.with_context(|| {
            format!(
                "failed to read main script {:?} for mod {}",
                main_path, meta.id
            )
        })?;
        let used_script_paths = collect_used_script_files(
            &meta.id,
            &path,
            archive_scripts_root.as_deref(),
            main_path,
            &script,
        )
        .with_context(|| format!("failed to discover used Luau files for mod {}", meta.id))?;
        for used_path in &used_script_paths {
            let source = fs::read_to_string(used_path).await.with_context(|| {
                format!(
                    "failed to read used script {:?} for mod {} during precheck",
                    used_path, meta.id
                )
            })?;
            precheck_luau_chunk(lua, &meta.id, used_path, &source)
                .with_context(|| format!("Luau precheck failed for mod {}", meta.id))?;
        }
        if let Ok(mut guard) = required_files.lock() {
            for used_path in used_script_paths {
                guard.insert(normalize_path(&used_path));
            }
        }

        set_engine_log_mod_id(lua, Some(meta.id.as_str()))?;
        let eval_result = lua
            .load(&script)
            .set_name(main_path.to_string_lossy().as_ref())
            .eval()
            .with_context(|| {
                format!(
                    "failed to evaluate Lua script {:?} for mod {}",
                    main_path, meta.id
                )
            });
        set_engine_log_mod_id(lua, None)?;
        eval_result?
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
            set_engine_log_mod_id(lua, Some(meta.id.as_str()))?;
            let load_result = if matches!(unload_payload, Value::Nil) {
                load.call::<()>(())
                    .with_context(|| format!("load() failed for mod {}", meta.id))
            } else {
                load.call::<()>(unload_payload)
                    .with_context(|| format!("load() failed for mod {}", meta.id))
            };
            set_engine_log_mod_id(lua, None)?;
            load_result?;
        }
    } else if let Ok(init_fn) = returned.get::<Function>("init") {
        modloader_debug!("Calling init() for mod {}", meta.id);
        set_engine_log_mod_id(lua, Some(meta.id.as_str()))?;
        let init_result = with_suspend_override(false, || init_fn.call::<()>(()));
        set_engine_log_mod_id(lua, None)?;
        if let Err(err) = init_result {
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

fn precheck_luau_chunk(lua: &Lua, mod_id: &str, script_path: &Path, source: &str) -> Result<()> {
    let chunk_name = script_path.to_string_lossy().to_string();
    lua.load(source)
        .set_name(&chunk_name)
        .into_function()
        .map_err(|err| {
            anyhow!(
                "precheck failed in mod '{}' for file '{}': syntax/type/static check error: {}",
                mod_id,
                chunk_name,
                err
            )
        })?;
    modloader_trace!(
        "Luau precheck passed for mod '{}' file '{}'",
        mod_id,
        chunk_name
    );
    Ok(())
}

fn collect_used_script_files(
    mod_id: &str,
    mod_root: &Path,
    archive_root: Option<&Path>,
    main_path: &Path,
    main_source: &str,
) -> Result<Vec<PathBuf>> {
    let mut ordered: Vec<PathBuf> = Vec::new();
    let mut visited: HashSet<PathBuf> = HashSet::new();
    let mut stack: Vec<(PathBuf, String)> =
        vec![(normalize_path(main_path), main_source.to_owned())];

    while let Some((current, source)) = stack.pop() {
        if !visited.insert(current.clone()) {
            continue;
        }
        ordered.push(current.clone());

        for request in parse_engine_require_literals(&source)? {
            let resolved = resolve_mod_relative_luau_path(mod_root, archive_root, &request)
                .with_context(|| {
                    format!(
                        "mod '{}' uses Engine.require('{}') in '{}' but the target could not be resolved",
                        mod_id,
                        request,
                        current.display()
                    )
                })?;
            let normalized_resolved = normalize_path(&resolved);
            if visited.contains(&normalized_resolved) {
                continue;
            }

            let required_source =
                std::fs::read_to_string(&normalized_resolved).with_context(|| {
                    format!(
                        "failed to read required file '{}' while analyzing mod '{}'",
                        normalized_resolved.display(),
                        mod_id
                    )
                })?;
            stack.push((normalized_resolved, required_source));
        }
    }

    Ok(ordered)
}

fn parse_engine_require_literals(source: &str) -> Result<Vec<String>> {
    let mut parser = Parser::new();
    parser
        .set_language(&tree_sitter_luau::LANGUAGE.into())
        .map_err(|err| {
            anyhow!("failed to initialize Lua parser for Engine.require analysis: {err}")
        })?;
    let tree = parser
        .parse(source, None)
        .ok_or_else(|| anyhow!("failed to parse Lua source while searching for Engine.require"))?;

    let root = tree.root_node();
    if root.has_error() {
        return Err(anyhow!(
            "Lua AST contains parse errors; cannot reliably inspect Engine.require calls"
        ));
    }

    let mut calls = Vec::new();
    let mut cursor = root.walk();
    let mut visit_stack = vec![root];

    while let Some(node) = visit_stack.pop() {
        if node.kind() == "function_call"
            && let Some(request) = extract_engine_require_argument(node, source)
        {
            calls.push(request);
        }

        for child in node.children(&mut cursor) {
            visit_stack.push(child);
        }
    }

    Ok(calls)
}

fn extract_engine_require_argument(call_node: tree_sitter::Node, source: &str) -> Option<String> {
    let call_text = call_node.utf8_text(source.as_bytes()).ok()?.trim();
    let mut prefixes = ["Engine.require(\"", "Engine.require('"];
    let prefix = prefixes
        .iter_mut()
        .find(|prefix| call_text.starts_with(**prefix))?;
    let quote = prefix.chars().last()?;
    let tail = &call_text[prefix.len()..];
    let end_idx = tail.find(quote)?;
    Some(tail[..end_idx].to_owned())
}

fn resolve_mod_relative_luau_path(
    mod_root: &Path,
    archive_root: Option<&Path>,
    requested_path: &str,
) -> Result<PathBuf> {
    let relative = Path::new(requested_path);
    if relative.is_absolute() {
        return Err(anyhow!(
            "Engine.require path must be relative, got '{}'",
            requested_path
        ));
    }

    for root in [Some(mod_root), archive_root].into_iter().flatten() {
        let candidate = root.join(relative);
        if candidate.exists() {
            let canonical = candidate.canonicalize().with_context(|| {
                format!(
                    "failed to canonicalize required path '{}'",
                    candidate.display()
                )
            })?;
            if canonical.starts_with(root) {
                return Ok(canonical);
            }
            return Err(anyhow!(
                "Engine.require path '{}' resolves outside mod root",
                requested_path
            ));
        }

        let with_ext = candidate.with_extension("luau");
        if with_ext.exists() {
            let canonical = with_ext.canonicalize().with_context(|| {
                format!(
                    "failed to canonicalize required path '{}'",
                    with_ext.display()
                )
            })?;
            if canonical.starts_with(root) {
                return Ok(canonical);
            }
            return Err(anyhow!(
                "Engine.require path '{}' resolves outside mod root",
                requested_path
            ));
        }
    }

    Err(anyhow!(
        "Engine.require target not found for '{}'",
        requested_path
    ))
}

fn resolve_script_path(
    mod_root: &Path,
    archive_root: Option<&Path>,
    script_name: &str,
) -> Option<PathBuf> {
    [Some(mod_root), archive_root]
        .into_iter()
        .flatten()
        .map(|root| root.join(script_name))
        .find(|path| path.is_file())
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
    let assets_root = mod_path.join("assets");
    if assets_root.exists()
        && let Ok(asset_files) = collect_mod_asset_files(&assets_root)
    {
        files.extend(asset_files);
    }
    files.push(normalize_path(&mod_path.join("assets.tar.zstd")));
    files.push(normalize_path(&mod_path.join("converts.tar.zstd")));
    files.push(normalize_path(&mod_path.join("script.tar.zstd")));

    files.sort();
    files.dedup();
    files
}

fn collect_mod_asset_files(assets_root: &Path) -> Result<Vec<PathBuf>> {
    let mut files = Vec::new();
    let mut stack = vec![assets_root.to_path_buf()];
    while let Some(path) = stack.pop() {
        for entry in std::fs::read_dir(&path)
            .with_context(|| format!("failed to read assets directory {}", path.display()))?
        {
            let entry = entry.with_context(|| {
                format!(
                    "failed to read assets directory entry in {}",
                    path.display()
                )
            })?;
            let candidate = entry.path();
            if candidate.is_dir() {
                stack.push(candidate);
            } else if candidate.is_file() {
                files.push(normalize_path(&candidate));
            }
        }
    }
    Ok(files)
}

fn prepare_script_archive_dir(mod_path: &Path) -> Result<Option<PathBuf>> {
    let archive_path = mod_path.join("script.tar.zstd");
    if !archive_path.is_file() {
        return Ok(None);
    }

    let target_dir = mod_path.join(".ccaster_script_archive");
    if target_dir.exists() {
        std::fs::remove_dir_all(&target_dir).with_context(|| {
            format!(
                "failed clearing extracted script archive directory {}",
                target_dir.display()
            )
        })?;
    }
    std::fs::create_dir_all(&target_dir).with_context(|| {
        format!(
            "failed creating extracted script archive directory {}",
            target_dir.display()
        )
    })?;
    extract_tar_zstd_into(&archive_path, &target_dir)?;
    Ok(Some(target_dir))
}

fn is_tar_zstd_archive(path: &Path) -> bool {
    path.is_file()
        && path
            .file_name()
            .and_then(|name| name.to_str())
            .is_some_and(|name| name.ends_with(".tar.zstd"))
}

fn prepare_archived_mod_dir(root: &Path, archive_path: &Path) -> Result<Option<(String, PathBuf)>> {
    let Some(file_name) = archive_path.file_name().and_then(|name| name.to_str()) else {
        return Ok(None);
    };
    let Some(mod_id) = file_name.strip_suffix(".tar.zstd").map(str::to_owned) else {
        return Ok(None);
    };
    if mod_id.is_empty() || mod_id == "addons" || mod_id == "converts" || mod_id == "cccaster" {
        return Ok(None);
    }

    let extract_root = root.join(".cccaster").join("addons_archives").join(&mod_id);
    if extract_root.exists() {
        std::fs::remove_dir_all(&extract_root).with_context(|| {
            format!(
                "failed clearing extracted addon archive directory {}",
                extract_root.display()
            )
        })?;
    }
    std::fs::create_dir_all(&extract_root).with_context(|| {
        format!(
            "failed creating extracted addon archive directory {}",
            extract_root.display()
        )
    })?;
    extract_tar_zstd_into(archive_path, &extract_root)?;
    let info_at_root = extract_root.join("info.json").is_file();
    if info_at_root {
        return Ok(Some((mod_id, extract_root)));
    }
    let nested_dirs: Vec<PathBuf> = std::fs::read_dir(&extract_root)
        .with_context(|| {
            format!(
                "failed reading extracted archive {}",
                extract_root.display()
            )
        })?
        .filter_map(|entry| entry.ok().map(|e| e.path()))
        .filter(|path| path.is_dir())
        .collect();
    if nested_dirs.len() == 1 && nested_dirs[0].join("info.json").is_file() {
        return Ok(Some((mod_id, nested_dirs[0].clone())));
    }
    Ok(Some((mod_id, extract_root)))
}

fn ensure_root_archive_available(root: &Path, archive_names: &[&str]) -> Result<()> {
    let should_extract = !root.join("addons").exists() || !root.join("converts").exists();
    if !should_extract {
        return Ok(());
    }
    let Some(archive_path) = archive_names
        .iter()
        .map(|name| root.join(name))
        .find(|candidate| candidate.is_file())
    else {
        return Ok(());
    };
    modloader_info!(
        "Detected missing addons/ or converts/; extracting root archive {}",
        archive_path.display()
    );
    extract_tar_zstd_into(&archive_path, root)
        .with_context(|| format!("failed extracting {}", archive_path.display()))
}

fn ensure_archived_directory_available(
    root: &Path,
    dir_name: &str,
    archive_names: &[&str],
) -> Result<()> {
    let target_dir = root.join(dir_name);
    if target_dir.exists() {
        return Ok(());
    }

    let Some(archive_path) = archive_names
        .iter()
        .map(|name| root.join(name))
        .find(|candidate| candidate.is_file())
    else {
        return Ok(());
    };

    modloader_info!(
        "Directory '{}' missing; extracting {}",
        dir_name,
        archive_path.display()
    );
    if archive_path
        .file_name()
        .and_then(|name| name.to_str())
        .is_some_and(|name| name == "cccaster.tar.zstd")
    {
        extract_tar_zstd_into(&archive_path, root)
            .with_context(|| format!("failed extracting {}", archive_path.display()))
    } else {
        extract_tar_zstd_into(&archive_path, &target_dir)
            .with_context(|| format!("failed extracting {}", archive_path.display()))
    }
}

fn extract_tar_zstd_into(archive_path: &Path, target_dir: &Path) -> Result<()> {
    let archive_file = std::fs::File::open(archive_path)
        .with_context(|| format!("failed opening archive {}", archive_path.display()))?;
    let mut decoder = zstd::stream::read::Decoder::new(archive_file)
        .with_context(|| format!("failed decoding archive {}", archive_path.display()))?;
    let mut decoded = Vec::new();
    use std::io::Read;
    decoder
        .read_to_end(&mut decoded)
        .with_context(|| format!("failed reading decoded archive {}", archive_path.display()))?;

    std::fs::create_dir_all(target_dir)
        .with_context(|| format!("failed creating target dir {}", target_dir.display()))?;
    let mut archive = tar::Archive::new(std::io::Cursor::new(decoded));
    archive
        .unpack(target_dir)
        .with_context(|| format!("failed unpacking archive into {}", target_dir.display()))?;
    Ok(())
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
            set_engine_log_mod_id(lua, Some(meta.id.as_str()))?;
            let post_result = with_suspend_override(false, || post_fn.call::<()>(()));
            set_engine_log_mod_id(lua, None)?;
            if let Err(err) = post_result {
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
            let quit_info = quit_fn.info();
            let quit_location = match (quit_info.short_src, quit_info.line_defined) {
                (Some(file), Some(line)) => format!("{file}:{line}"),
                (Some(file), None) => file,
                _ => "<unknown location>".to_string(),
            };
            set_engine_log_mod_id(lua, Some(loaded.meta.id.as_str()))?;
            let quit_result = quit_fn.call::<()>(());
            set_engine_log_mod_id(lua, None)?;
            if let Err(err) = quit_result {
                modloader_error!(
                    "quit() failed for mod {} at {}: {}",
                    loaded.meta.id,
                    quit_location,
                    err
                );
            }
        }
    }
    Ok(())
}
