/// Engine environment and local virtual filesystem APIs.
use anyhow::Context;
use std::collections::HashMap;
use std::fs::File;
use std::io::{Cursor, Read};

use super::overlay_vfs;
use crate::runtime_args;

#[derive(Clone)]
enum LocalWriteMode {
    Write,
    Append,
}

impl LocalWriteMode {
    fn from_lua_value(raw: Option<String>) -> mlua::Result<Self> {
        match raw
            .as_deref()
            .map(str::trim)
            .filter(|value| !value.is_empty())
            .map(|value| value.to_ascii_lowercase())
            .as_deref()
        {
            Some("write") | Some("w") => Ok(Self::Write),
            Some("append") | Some("a") => Ok(Self::Append),
            Some(other) => Err(mlua::Error::runtime(format!(
                "unsupported Engine.fs.local.write mode '{other}'"
            ))),
            None => Err(mlua::Error::runtime(
                "unsupported Engine.fs.local.write mode 'empty'".to_string(),
            )),
        }
    }
}

#[derive(Default)]
struct LocalVfs {
    entries: HashMap<String, Vec<u8>>,
}

struct DeferredLocalVfs {
    archive_path: std::path::PathBuf,
    state: Option<LocalVfs>,
}

impl DeferredLocalVfs {
    fn new(archive_path: std::path::PathBuf) -> Self {
        Self {
            archive_path,
            state: None,
        }
    }

    fn ensure_loaded(&mut self) -> anyhow::Result<&mut LocalVfs> {
        if self.state.is_none() {
            let loaded = load_or_initialize_vfs(&self.archive_path).with_context(|| {
                format!("failed to initialize local VFS at {:?}", self.archive_path)
            })?;
            self.state = Some(loaded);
        }

        self.state
            .as_mut()
            .context("local VFS was not initialized after loading")
    }
}

pub(super) fn install_engine_env_api(
    lua: &mlua::Lua,
    engine_table: &mlua::Table,
) -> anyhow::Result<()> {
    let env_table = lua.create_table()?;
    for (key, value) in std::env::vars() {
        if key.starts_with("ADDON_") {
            env_table.set(key, value)?;
        }
    }
    engine_table.set("env", env_table)?;
    Ok(())
}

/// Installs Engine.fs.local APIs scoped to the currently loading mod directory.
pub(super) fn register_engine_fs_local(
    lua: &mlua::Lua,
    mod_path: std::path::PathBuf,
) -> anyhow::Result<()> {
    let globals = lua.globals();
    let engine_table: mlua::Table = globals.get("Engine")?;

    let fs_table = match engine_table.get::<mlua::Table>("fs") {
        Ok(table) => table,
        Err(_) => lua.create_table()?,
    };

    let mode_table = lua.create_table()?;
    mode_table.set("write", "write")?;
    mode_table.set("append", "append")?;
    fs_table.set("mode", mode_table)?;

    let archive_path = mod_path.join("vfs.tar.zstd");
    let vfs_state = std::sync::Arc::new(std::sync::Mutex::new(DeferredLocalVfs::new(
        archive_path.clone(),
    )));

    let local_table = lua.create_table()?;

    let read_state = vfs_state.clone();
    let read_fn = lua.create_function(move |_, path: String| {
        let normalized = normalize_vfs_path(&path)?;
        let mut guard = read_state
            .lock()
            .map_err(|_| mlua::Error::runtime("local VFS mutex poisoned"))?;
        let loaded = guard
            .ensure_loaded()
            .map_err(|err| mlua::Error::runtime(err.to_string()))?;

        match loaded.entries.get(&normalized) {
            Some(bytes) => Ok(Some(String::from_utf8_lossy(bytes).into_owned())),
            None => Ok(None),
        }
    })?;

    let write_state = vfs_state.clone();
    let write_archive_path = archive_path.clone();
    let write_fn = lua.create_function(
        move |_, (path, content, mode): (String, String, Option<String>)| {
            let normalized = normalize_vfs_path(&path)?;
            let selected_mode = LocalWriteMode::from_lua_value(mode)?;

            let mut guard = write_state
                .lock()
                .map_err(|_| mlua::Error::runtime("local VFS mutex poisoned"))?;
            let loaded = guard
                .ensure_loaded()
                .map_err(|err| mlua::Error::runtime(err.to_string()))?;

            match selected_mode {
                LocalWriteMode::Write => {
                    loaded.entries.insert(normalized, content.into_bytes());
                }
                LocalWriteMode::Append => {
                    loaded
                        .entries
                        .entry(normalized)
                        .or_default()
                        .extend_from_slice(content.as_bytes());
                }
            }

            persist_vfs(&write_archive_path, loaded)
                .map_err(|err| mlua::Error::runtime(err.to_string()))?;
            Ok(true)
        },
    )?;

    local_table.set("read", read_fn)?;
    local_table.set("write", write_fn)?;
    // NOTE: Immanent = local
    fs_table.set("immanent", local_table)?;

    let global_table = lua.create_table()?;
    let global_read = lua.create_function(|_, path: String| {
        let normalized = normalize_vfs_path(&path)?;
        Ok(overlay_vfs::read_global(&normalized))
    })?;
    let global_write = lua.create_function(
        |_, (path, content, mode): (String, String, Option<String>)| {
            let normalized = normalize_vfs_path(&path)?;
            let selected_mode = LocalWriteMode::from_lua_value(mode)?;
            if runtime_args().sandbox {
                crate::modloader_debug!(
                    "Sandbox enabled: skipping Engine.fs.global.write for '{}'",
                    normalized
                );
                return Ok(false);
            }
            overlay_vfs::write_global(
                normalized,
                content.into_bytes(),
                matches!(selected_mode, LocalWriteMode::Append),
            )
            .map_err(|err| mlua::Error::runtime(err.to_string()))?;
            Ok(true)
        },
    )?;
    global_table.set("read", global_read)?;
    global_table.set("write", global_write)?;
    fs_table.set("global", global_table)?;

    engine_table.set("fs", fs_table)?;
    Ok(())
}

fn normalize_vfs_path(path: &str) -> mlua::Result<String> {
    let trimmed = path.trim();
    if trimmed.is_empty() {
        return Err(mlua::Error::runtime("path cannot be empty"));
    }

    let parsed = std::path::Path::new(trimmed);
    if parsed.is_absolute() {
        return Err(mlua::Error::runtime("absolute paths are not allowed"));
    }

    let mut parts: Vec<String> = Vec::new();
    for component in parsed.components() {
        match component {
            std::path::Component::Normal(value) => {
                parts.push(value.to_string_lossy().to_string());
            }
            std::path::Component::CurDir => {}
            _ => {
                return Err(mlua::Error::runtime(
                    "path traversal and parent components are not allowed",
                ));
            }
        }
    }

    if parts.is_empty() {
        return Err(mlua::Error::runtime(
            "path cannot resolve to an empty value",
        ));
    }

    Ok(parts.join("/"))
}

fn load_or_initialize_vfs(archive_path: &std::path::Path) -> anyhow::Result<LocalVfs> {
    if !archive_path.exists() {
        let state = LocalVfs::default();
        persist_vfs(archive_path, &state)?;
        return Ok(state);
    }

    let file = File::open(archive_path)
        .with_context(|| format!("failed to open VFS archive {}", archive_path.display()))?;
    let mut decoder = zstd::stream::read::Decoder::new(file)
        .with_context(|| format!("failed to decode VFS archive {}", archive_path.display()))?;
    let mut decoded = Vec::new();
    decoder.read_to_end(&mut decoded).with_context(|| {
        format!(
            "failed reading decoded VFS payload from {}",
            archive_path.display()
        )
    })?;

    let mut archive = tar::Archive::new(Cursor::new(decoded));
    let mut state = LocalVfs::default();
    for entry in archive
        .entries()
        .context("failed to iterate VFS archive entries")?
    {
        let mut entry = entry.context("failed to read VFS archive entry")?;
        if !entry.header().entry_type().is_file() {
            continue;
        }

        let path = entry
            .path()
            .context("failed to read VFS archive entry path")?;
        let key = path.to_string_lossy().replace('\\', "/");

        let mut content = Vec::new();
        entry
            .read_to_end(&mut content)
            .context("failed to read VFS entry content")?;
        state.entries.insert(key, content);
    }

    Ok(state)
}

fn persist_vfs(archive_path: &std::path::Path, state: &LocalVfs) -> anyhow::Result<()> {
    let mut tar_payload = Vec::new();
    {
        let mut builder = tar::Builder::new(&mut tar_payload);
        let mut sorted_paths: Vec<&String> = state.entries.keys().collect();
        sorted_paths.sort();

        for path in sorted_paths {
            let content = state
                .entries
                .get(path)
                .with_context(|| format!("missing content for VFS key {path}"))?;
            let mut header = tar::Header::new_gnu();
            header.set_size(content.len() as u64);
            header.set_mode(0o644);
            header.set_cksum();
            builder
                .append_data(&mut header, path.as_str(), Cursor::new(content))
                .with_context(|| format!("failed writing VFS entry {path}"))?;
        }
        builder
            .finish()
            .context("failed finalizing VFS tar archive")?;
    }

    let file = File::create(archive_path)
        .with_context(|| format!("failed creating VFS archive {}", archive_path.display()))?;
    let mut encoder = zstd::stream::write::Encoder::new(file, 3).with_context(|| {
        format!(
            "failed to create zstd encoder for {}",
            archive_path.display()
        )
    })?;
    std::io::copy(&mut Cursor::new(tar_payload), &mut encoder)
        .with_context(|| format!("failed writing VFS archive {}", archive_path.display()))?;
    encoder
        .finish()
        .context("failed finalizing zstd VFS archive")?;
    Ok(())
}
