/// Engine environment and local/global virtual filesystem APIs.
use anyhow::Context;
use easy_fuser::templates::DefaultFuseHandler;
use easy_fuser::templates::mirror_fs::{MirrorFs, MirrorFsTrait};
use glob::Pattern;
use std::collections::HashMap;
use std::fs::File;
use std::io::{Cursor, Read};
use std::path::{Component, Path, PathBuf};
use std::sync::{Arc, Mutex};

type AssetConverterFn = fn(&[u8]) -> anyhow::Result<Vec<u8>>;

static GLOBAL_ASSET_CONVERTERS: once_cell::sync::Lazy<Mutex<HashMap<String, AssetConverterFn>>> =
    once_cell::sync::Lazy::new(|| Mutex::new(HashMap::new()));

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

#[derive(Default)]
struct GlobalVfs {
    entries: HashMap<String, Vec<u8>>,
}

struct GlobalPassthroughFs {
    launcher_root: PathBuf,
    whitelist: Vec<Pattern>,
    vfs: GlobalVfs,
}

impl GlobalPassthroughFs {
    fn new(launcher_root: PathBuf, whitelist_patterns: Vec<String>) -> anyhow::Result<Self> {
        let mut whitelist = Vec::new();
        for pattern in whitelist_patterns {
            whitelist.push(
                Pattern::new(pattern.trim())
                    .with_context(|| format!("invalid whitelist glob pattern '{}'", pattern))?,
            );
        }

        let mirror = MirrorFs::new(launcher_root.clone(), DefaultFuseHandler::new());
        let _ = mirror.source_dir();

        Ok(Self {
            launcher_root,
            whitelist,
            vfs: GlobalVfs::default(),
        })
    }

    fn is_whitelisted(&self, relative_path: &str) -> bool {
        self.whitelist.iter().any(|pattern| {
            pattern.matches(relative_path)
                || relative_path
                    .rsplit('/')
                    .next()
                    .is_some_and(|name| pattern.matches(name))
        })
    }

    fn read_text(&self, relative_path: &str) -> anyhow::Result<Option<String>> {
        if !self.is_whitelisted(relative_path)
            && let Some(content) = self.vfs.entries.get(relative_path)
        {
            return Ok(Some(String::from_utf8_lossy(content).into_owned()));
        }

        let host_path = resolve_path_within_launcher(&self.launcher_root, relative_path)?;
        if !host_path.exists() {
            return Ok(None);
        }

        let bytes = std::fs::read(&host_path)
            .with_context(|| format!("failed to read '{}'", host_path.display()))?;
        Ok(Some(String::from_utf8_lossy(&bytes).into_owned()))
    }

    fn write_text(
        &mut self,
        relative_path: &str,
        content: &str,
        mode: LocalWriteMode,
    ) -> anyhow::Result<bool> {
        if self.is_whitelisted(relative_path) {
            return Ok(false);
        }

        match mode {
            LocalWriteMode::Write => {
                self.vfs
                    .entries
                    .insert(relative_path.to_string(), content.as_bytes().to_vec());
            }
            LocalWriteMode::Append => {
                self.vfs
                    .entries
                    .entry(relative_path.to_string())
                    .or_default()
                    .extend_from_slice(content.as_bytes());
            }
        }

        Ok(true)
    }

    fn import_asset(
        &mut self,
        host_path: &Path,
        relative_path: &str,
        ignore_patterns: &[Pattern],
        convert_map: &HashMap<String, HashMap<String, String>>,
    ) -> anyhow::Result<()> {
        if self.is_whitelisted(relative_path) {
            return Ok(());
        }
        if ignore_patterns.iter().any(|pattern| {
            pattern.matches(relative_path)
                || relative_path
                    .rsplit('/')
                    .next()
                    .is_some_and(|name| pattern.matches(name))
        }) {
            return Ok(());
        }

        let bytes = std::fs::read(host_path)
            .with_context(|| format!("failed reading asset file '{}'", host_path.display()))?;

        let extension = Path::new(relative_path)
            .extension()
            .map(|value| value.to_string_lossy().to_ascii_lowercase());
        if let Some(from_ext) = extension
            && let Some(targets) = convert_map.get(from_ext.as_str())
        {
            let converters = GLOBAL_ASSET_CONVERTERS
                .lock()
                .map_err(|_| anyhow::anyhow!("asset converter map mutex poisoned"))?;
            for (to_ext, converter_name) in targets {
                let converter = converters.get(converter_name).with_context(|| {
                    format!("missing asset converter '{converter_name}' for '{relative_path}'")
                })?;
                let converted = converter(&bytes).with_context(|| {
                    format!("asset conversion failed for '{}'", host_path.display())
                })?;
                let converted_relative =
                    replace_extension(relative_path, to_ext).with_context(|| {
                        format!(
                            "failed to rewrite extension of '{}' to '{}'",
                            relative_path, to_ext
                        )
                    })?;
                self.vfs.entries.insert(converted_relative, converted);
            }
            return Ok(());
        }

        self.vfs.entries.insert(relative_path.to_string(), bytes);
        Ok(())
    }
}

static GLOBAL_FS: once_cell::sync::Lazy<Arc<Mutex<GlobalPassthroughFs>>> =
    once_cell::sync::Lazy::new(|| {
        let launcher_root = std::env::current_dir().unwrap_or_else(|_| PathBuf::from("."));
        let whitelist_path = launcher_root.join("whitelist.json");
        let patterns = load_whitelist_patterns(&whitelist_path).unwrap_or_default();
        let global_fs = GlobalPassthroughFs::new(launcher_root, patterns).unwrap_or_else(|_| {
            GlobalPassthroughFs {
                launcher_root: PathBuf::from("."),
                whitelist: Vec::new(),
                vfs: GlobalVfs::default(),
            }
        });
        Arc::new(Mutex::new(global_fs))
    });

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

pub(super) fn load_global_assets_for_mod(
    mod_path: &Path,
    ignore_globs: &[String],
    convert_map: &HashMap<String, HashMap<String, String>>,
) -> anyhow::Result<()> {
    let assets_root = mod_path.join("assets");
    if !assets_root.exists() {
        return Ok(());
    }

    let mut ignore_patterns = Vec::new();
    for raw_pattern in ignore_globs {
        ignore_patterns
            .push(Pattern::new(raw_pattern).with_context(|| {
                format!("invalid assets.ignore glob pattern '{}'", raw_pattern)
            })?);
    }

    let launcher_root = std::env::current_dir().context("failed to resolve launcher path")?;
    let mut stack = vec![assets_root.clone()];

    while let Some(current) = stack.pop() {
        for entry in std::fs::read_dir(&current)
            .with_context(|| format!("failed reading assets directory '{}'", current.display()))?
        {
            let entry = entry.with_context(|| {
                format!("failed reading assets entry in '{}'", current.display())
            })?;
            let path = entry.path();

            if path.is_dir() {
                stack.push(path);
                continue;
            }
            if !path.is_file() {
                continue;
            }

            let relative_to_launcher = path
                .strip_prefix(&launcher_root)
                .ok()
                .map(|value| value.to_string_lossy().replace('\\', "/"))
                .context("asset path escaped launcher directory")?;

            let mut guard = GLOBAL_FS
                .lock()
                .map_err(|_| anyhow::anyhow!("global filesystem mutex poisoned"))?;
            guard.import_asset(&path, &relative_to_launcher, &ignore_patterns, convert_map)?;
        }
    }

    Ok(())
}

/// Installs Engine.fs.local and Engine.fs.global APIs.
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
    fs_table.set("local", local_table)?;

    let global_table = lua.create_table()?;

    let global_read_fn = lua.create_function(move |_, path: String| {
        let normalized = normalize_vfs_path(&path)?;
        let guard = GLOBAL_FS
            .lock()
            .map_err(|_| mlua::Error::runtime("global VFS mutex poisoned"))?;
        guard
            .read_text(&normalized)
            .map_err(|err| mlua::Error::runtime(err.to_string()))
    })?;

    let global_write_fn = lua.create_function(
        move |_, (path, content, mode): (String, String, Option<String>)| {
            let normalized = normalize_vfs_path(&path)?;
            let selected_mode = LocalWriteMode::from_lua_value(mode)?;
            let mut guard = GLOBAL_FS
                .lock()
                .map_err(|_| mlua::Error::runtime("global VFS mutex poisoned"))?;
            guard
                .write_text(&normalized, &content, selected_mode)
                .map_err(|err| mlua::Error::runtime(err.to_string()))
        },
    )?;

    global_table.set("read", global_read_fn)?;
    global_table.set("write", global_write_fn)?;
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

fn resolve_path_within_launcher(
    launcher_root: &Path,
    relative_path: &str,
) -> anyhow::Result<PathBuf> {
    let mut result = launcher_root.to_path_buf();
    for component in Path::new(relative_path).components() {
        match component {
            Component::Normal(segment) => result.push(segment),
            Component::CurDir => {}
            _ => {
                return Err(anyhow::anyhow!(
                    "path traversal is not allowed: '{}'",
                    relative_path
                ));
            }
        }
    }
    Ok(result)
}

fn load_whitelist_patterns(path: &Path) -> anyhow::Result<Vec<String>> {
    if !path.exists() {
        return Ok(Vec::new());
    }
    let data = std::fs::read(path)
        .with_context(|| format!("failed reading whitelist file '{}'", path.display()))?;
    serde_json::from_slice::<Vec<String>>(&data)
        .with_context(|| format!("failed parsing whitelist file '{}'", path.display()))
}

fn replace_extension(relative_path: &str, target_extension: &str) -> anyhow::Result<String> {
    let cleaned_extension = target_extension.trim().trim_start_matches('.').to_string();
    if cleaned_extension.is_empty() {
        return Err(anyhow::anyhow!("target extension cannot be empty"));
    }

    let path = Path::new(relative_path);
    let stem = path
        .file_stem()
        .map(|value| value.to_string_lossy().to_string())
        .context("source path is missing file stem")?;
    let parent = path.parent().filter(|value| !value.as_os_str().is_empty());

    let file_name = format!("{stem}.{cleaned_extension}");
    let rebuilt = if let Some(parent_path) = parent {
        parent_path.join(file_name)
    } else {
        PathBuf::from(file_name)
    };
    Ok(rebuilt.to_string_lossy().replace('\\', "/"))
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
