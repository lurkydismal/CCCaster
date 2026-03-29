use crate::patch::{PatchEntry, Patches};
use crate::types::{Dependency, ModMeta, RawModInfo};
use crate::{modloader_error, modloader_warning};
use anyhow::{Result, anyhow};
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

    // Step 1: List all mods in addons/
    let mut dir = fs::read_dir(addons_dir).await?;
    while let Some(entry) = dir.next_entry().await? {
        let path = entry.path();
        if path.is_dir() {
            // Sanitize path to avoid traversal attacks
            if !is_safe_path(addons_dir, &path) {
                modloader_warning!("Skipping unsafe directory path: {:?}", path);
                continue;
            }

            // Read and validate info.json
            let info_path = path.join("info.json");
            let info_data = fs::read(&info_path).await;
            if info_data.is_err() {
                modloader_error!("Failed to read info.json for mod at {:?}, skipping", path);
                continue;
            }
            let raw: RawModInfo = serde_json::from_slice(&info_data.unwrap())?;
            // Parse version
            let version = Version::parse(&raw.version)?;
            // Parse dependencies
            let mut deps = Vec::new();
            for rd in raw.dependencies {
                let ver_req = VersionReq::parse(&rd.version_req)?;
                deps.push(Dependency {
                    id: rd.id,
                    version_req: ver_req,
                    optional: rd.optional,
                });
            }
            let meta = ModMeta {
                id: raw.id.clone(),
                version,
                dependencies: deps,
                api_version: raw.api_version,
                events: raw.events.clone(),
            };
            crate::modloader_info!(
                "Discovered mod {} (api_version={}, events={})",
                meta.id,
                meta.api_version,
                meta.events.len()
            );

            mod_entries.push((meta, path.clone()));
        }
    }

    // Step 2: Apply patches and parse patch.json if present
    // We create a map of mod_id -> Patches to keep them alive.
    let mut patches_map: HashMap<String, Patches> = HashMap::new();
    for (meta, path) in &mod_entries {
        let patch_path = path.join("patch.json");
        if patch_path.exists() {
            let patch_data = fs::read(&patch_path).await?;
            let patch_entries: Vec<PatchEntry> = serde_json::from_slice(&patch_data)?;
            let patches = Patches::new(&patch_entries);
            patches_map.insert(meta.id.clone(), patches);
        }
    }

    // Step 3: Build dependency graph
    let mut graph = Graph::<usize, ()>::new();
    let mut indices: HashMap<String, petgraph::graph::NodeIndex> = HashMap::new();
    for (i, (meta, _path)) in mod_entries.iter().enumerate() {
        indices.insert(meta.id.clone(), graph.add_node(i));
    }
    // Add edges for dependencies
    mod_entries.iter().for_each(|(meta, _path)| {
        for dep in &meta.dependencies {
            if let Some(&dep_idx) = indices.get(&dep.id) {
                // Check version constraint
                let target_index = *graph.node_weight(dep_idx).unwrap();
                let target_meta = &mod_entries[target_index].0;
                if dep.version_req.matches(&target_meta.version) {
                    // add edge from dependency to this mod
                    let this_idx = indices[&meta.id];
                    graph.add_edge(dep_idx, this_idx, ());
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

    // Step 4: Detect cycles and compute load order
    let sorted = toposort(&graph, None).map_err(|cycle| {
        anyhow!(
            "Circular dependency detected involving index: {:?}",
            cycle.node_id()
        )
    })?;
    let mut load_order: Vec<usize> = Vec::new();
    for idx in sorted {
        let mod_index = *graph.node_weight(idx).unwrap();
        load_order.push(mod_index);
    }

    // Step 5: Initialize Lua and sandbox
    let lua = Lua::new();
    lua.sandbox(true)?;
    let globals = lua.globals();
    // Create global Engine table
    let engine_table = lua.create_table()?;
    globals.set("Engine", engine_table)?;

    // Step 6: Load mods in order
    for &mod_index in &load_order {
        let (meta, path) = &mod_entries[mod_index];
        // Load main.luau script
        let main_path = path.join("main.luau");
        let script = fs::read_to_string(&main_path).await?;
        // Execute script, expecting it returns a table
        let returned: Table = lua.load(&script).eval()?;
        // Register returned table under Engine
        let engine_table: Table = globals.get("Engine")?;
        engine_table.set(meta.id.clone(), returned.clone())?;
        // Call init callback if present
        if let Ok(init_fn) = returned.get::<Function>("init") {
            let _ = init_fn.call::<()>(());
        }
    }

    // Step 7: Call post_init on all mods
    let engine_table: Table = globals.get("Engine")?;
    for &mod_index in &load_order {
        let (meta, _path) = &mod_entries[mod_index];
        let mod_table: Table = engine_table.get(meta.id.clone())?;
        if let Ok(post_fn) = mod_table.get::<Function>("post_init") {
            let _ = post_fn.call::<()>(());
        }
    }

    Ok(())
}

/// Checks if the child path is inside the base directory (to prevent traversal).
fn is_safe_path(base: &Path, child: &Path) -> bool {
    match (base.canonicalize(), child.canonicalize()) {
        (Ok(b), Ok(c)) => c.starts_with(&b),
        _ => false,
    }
}
