use crate::patch::{PatchEntry, Patches};
use crate::types::{Dependency, ModMeta, RawModInfo};
use crate::{modloader_debug, modloader_error, modloader_info, modloader_trace, modloader_warning};
use anyhow::{Context, Result, anyhow};
use mlua::{Function, Lua, Table};
use petgraph::algo::toposort;
use petgraph::graph::Graph;
use semver::{Version, VersionReq};
use std::collections::HashMap;
use std::path::{Path, PathBuf};
use tokio::fs;

/// Scans the 'addons' directory, loads mods, resolves dependencies, and initializes mods.
pub async fn load_mods_from_addons() -> Result<()> {
    let addons_dir = Path::new("addons");
    let mut mod_entries: Vec<(ModMeta, PathBuf)> = Vec::new();

    modloader_info!("Starting mod discovery in {:?}", addons_dir);

    // Step 1: List all mods in addons/
    let mut dir = fs::read_dir(addons_dir)
        .await
        .with_context(|| format!("unable to open addons directory at {:?}", addons_dir))?;
    while let Some(entry) = dir
        .next_entry()
        .await
        .with_context(|| format!("failed while iterating entries in {:?}", addons_dir))?
    {
        let path = entry.path();
        modloader_trace!("Inspecting addon entry at {:?}", path);
        if path.is_dir() {
            // Sanitize path to avoid traversal attacks
            if !is_safe_path(addons_dir, &path) {
                modloader_warning!("Skipping unsafe directory path: {:?}", path);
                continue;
            }
            modloader_debug!("Accepted addon directory {:?}", path);

            // Read and validate info.json
            let info_path = path.join("info.json");
            modloader_trace!("Reading manifest {:?}", info_path);
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
            modloader_debug!(
                "Loaded manifest for mod id={} version={} dependencies={}",
                raw.id,
                raw.version,
                raw.dependencies.len()
            );
            // Parse version
            let version = match Version::parse(&raw.version) {
                Ok(version) => version,
                Err(err) => {
                    modloader_error!(
                        "Invalid version '{}' in {:?} for mod {}: {}. Skipping this addon.",
                        raw.version,
                        info_path,
                        raw.id,
                        err
                    );
                    continue;
                }
            };
            // Parse dependencies
            let mut deps = Vec::new();
            let mut dep_parse_failed = false;
            for rd in raw.dependencies {
                modloader_trace!(
                    "Parsing dependency for mod {} => id={}, req={}, optional={}",
                    raw.id,
                    rd.id,
                    rd.version_req,
                    rd.optional
                );
                let ver_req = match VersionReq::parse(&rd.version_req) {
                    Ok(ver_req) => ver_req,
                    Err(err) => {
                        modloader_error!(
                            "Invalid dependency version requirement '{}' in {:?} (mod {}, dependency {}): {}. Skipping this addon.",
                            rd.version_req,
                            info_path,
                            raw.id,
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
                modloader_warning!(
                    "Dependency parsing failed for mod {} in {:?}; addon will be skipped.",
                    raw.id,
                    path
                );
                continue;
            }
            let meta = ModMeta {
                id: raw.id.clone(),
                version,
                dependencies: deps,
                api_version: raw.api_version,
                events: raw.events.clone(),
            };
            modloader_info!(
                "Discovered mod {} (api_version={}, events={})",
                meta.id,
                meta.api_version,
                meta.events.len()
            );
            modloader_trace!(
                "Mod {} details: version={}, deps={}, events={:?}",
                meta.id,
                meta.version,
                meta.dependencies.len(),
                meta.events
            );

            mod_entries.push((meta, path.clone()));
        } else {
            modloader_trace!("Skipping non-directory addon entry {:?}", path);
        }
    }

    modloader_info!("Finished discovery: {} candidate mods", mod_entries.len());

    // Step 2: Apply patches and parse patch.json if present
    // We create a map of mod_id -> Patches to keep them alive.
    let mut patches_map: HashMap<String, Patches> = HashMap::new();
    for (meta, path) in &mod_entries {
        let patch_path = path.join("patch.json");
        modloader_trace!("Checking for patch file {:?}", patch_path);
        if patch_path.exists() {
            modloader_debug!("Loading patches for mod {} from {:?}", meta.id, patch_path);
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
            modloader_info!(
                "Applying {} patch entries for mod {}",
                patch_entries.len(),
                meta.id
            );
            let patches = Patches::new(&patch_entries);
            patches_map.insert(meta.id.clone(), patches);
        } else {
            modloader_trace!("No patch file found for mod {}", meta.id);
        }
    }
    modloader_debug!("Patch map initialized for {} mods", patches_map.len());

    // Step 3: Build dependency graph
    let mut graph = Graph::<usize, ()>::new();
    let mut indices: HashMap<String, petgraph::graph::NodeIndex> = HashMap::new();
    for (i, (meta, _path)) in mod_entries.iter().enumerate() {
        modloader_trace!("Adding graph node {} => {}", i, meta.id);
        indices.insert(meta.id.clone(), graph.add_node(i));
    }
    modloader_debug!("Dependency graph initialized with {} nodes", indices.len());
    // Add edges for dependencies
    mod_entries.iter().for_each(|(meta, _path)| {
        for dep in &meta.dependencies {
            modloader_trace!(
                "Evaluating dependency edge: mod={} depends_on={} req={} optional={}",
                meta.id,
                dep.id,
                dep.version_req,
                dep.optional
            );
            if let Some(&dep_idx) = indices.get(&dep.id) {
                // Check version constraint
                let target_index = *graph.node_weight(dep_idx).unwrap();
                let target_meta = &mod_entries[target_index].0;
                if dep.version_req.matches(&target_meta.version) {
                    // add edge from dependency to this mod
                    let this_idx = indices[&meta.id];
                    graph.add_edge(dep_idx, this_idx, ());
                    modloader_debug!(
                        "Dependency satisfied: {} -> {} ({})",
                        dep.id,
                        meta.id,
                        dep.version_req
                    );
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
            } else {
                modloader_trace!(
                    "Optional dependency {} for mod {} is not present; continuing",
                    dep.id,
                    meta.id
                );
            }
        }
    });

    modloader_debug!("Dependency graph has {} edges", graph.edge_count());

    // Step 4: Detect cycles and compute load order
    modloader_info!("Resolving dependency order via topological sort");
    let sorted = toposort(&graph, None).map_err(|cycle| {
        anyhow!(
            "Circular dependency detected involving index: {:?}",
            cycle.node_id()
        )
    })?;
    let mut load_order: Vec<usize> = Vec::new();
    for idx in sorted {
        let mod_index = *graph.node_weight(idx).unwrap();
        modloader_trace!(
            "Toposort produced graph index {:?} => mod index {}",
            idx,
            mod_index
        );
        load_order.push(mod_index);
    }
    let ordered_mod_ids: Vec<&str> = load_order
        .iter()
        .map(|&i| mod_entries[i].0.id.as_str())
        .collect();
    modloader_info!("Resolved load order: {:?}", ordered_mod_ids);

    // Step 5: Initialize Lua and sandbox
    modloader_info!("Initializing Lua runtime with sandbox enabled");
    let lua = Lua::new();
    lua.sandbox(true)?;
    let globals = lua.globals();
    // Create global Engine table
    let engine_table = lua.create_table()?;
    globals.set("Engine", engine_table)?;
    modloader_debug!("Global Engine table registered");

    // Step 6: Load mods in order
    for &mod_index in &load_order {
        let (meta, path) = &mod_entries[mod_index];
        modloader_info!("Loading mod {} from {:?}", meta.id, path);
        // Load main.luau script
        let main_path = path.join("main.luau");
        modloader_trace!("Reading script {:?}", main_path);
        let script = fs::read_to_string(&main_path).await.with_context(|| {
            format!(
                "failed to read main script {:?} for mod {}",
                main_path, meta.id
            )
        })?;
        modloader_debug!("Read {} bytes of script for mod {}", script.len(), meta.id);
        // Execute script, expecting it returns a table
        let returned: Table = lua.load(&script).eval().with_context(|| {
            format!(
                "failed to evaluate Lua script {:?} for mod {}",
                main_path, meta.id
            )
        })?;
        modloader_debug!("Script for mod {} evaluated successfully", meta.id);
        // Register returned table under Engine
        let engine_table: Table = globals.get("Engine")?;
        engine_table.set(meta.id.clone(), returned.clone())?;
        modloader_trace!("Registered Engine.{} table", meta.id);
        // Call init callback if present
        if let Ok(init_fn) = returned.get::<Function>("init") {
            modloader_debug!("Calling init() for mod {}", meta.id);
            if let Err(err) = init_fn.call::<()>(()) {
                modloader_error!("init() failed for mod {}: {}", meta.id, err);
            }
        } else {
            modloader_trace!("Mod {} has no init() callback", meta.id);
        }
    }

    // Step 7: Call post_init on all mods
    modloader_info!("Executing post_init callbacks");
    let engine_table: Table = globals.get("Engine")?;
    for &mod_index in &load_order {
        let (meta, _path) = &mod_entries[mod_index];
        let mod_table: Table = engine_table.get(meta.id.clone())?;
        if let Ok(post_fn) = mod_table.get::<Function>("post_init") {
            modloader_debug!("Calling post_init() for mod {}", meta.id);
            if let Err(err) = post_fn.call::<()>(()) {
                modloader_error!("post_init() failed for mod {}: {}", meta.id, err);
            }
        } else {
            modloader_trace!("Mod {} has no post_init() callback", meta.id);
        }
    }

    modloader_info!("Mod loading completed successfully");

    Ok(())
}

/// Checks if the child path is inside the base directory (to prevent traversal).
fn is_safe_path(base: &Path, child: &Path) -> bool {
    match (base.canonicalize(), child.canonicalize()) {
        (Ok(b), Ok(c)) => {
            let safe = c.starts_with(&b);
            modloader_trace!("is_safe_path base={:?} child={:?} safe={}", b, c, safe);
            safe
        }
        (base_result, child_result) => {
            modloader_warning!(
                "Unable to canonicalize paths for safety check (base_ok={}, child_ok={})",
                base_result.is_ok(),
                child_result.is_ok()
            );
            false
        }
    }
}
