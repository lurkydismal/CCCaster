use anyhow::{Context, Result, anyhow};
use globset::{Glob, GlobMatcher};
use once_cell::sync::OnceCell;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::ffi::CString;
use std::fs;
use std::io::{Cursor, Read};
use std::path::{Component, Path, PathBuf};
use std::sync::{Arc, Mutex};
use std::time::{SystemTime, UNIX_EPOCH};
use wasmtime::{Engine, Instance, Memory, Module, Store, TypedFunc};

#[derive(Clone)]
struct ConverterEntry {
    id: String,
    matcher: GlobMatcher,
    wasm_path: PathBuf,
}

#[derive(Clone)]
struct SelectedAsset {
    rel_unix: String,
    source_hash_hex: String,
    mtime: u64,
    data: Vec<u8>,
}

pub struct OverlayRegistry {
    launcher_root: PathBuf,
    converters: Vec<ConverterEntry>,
}

impl OverlayRegistry {
    fn new(launcher_root: PathBuf) -> Result<Self> {
        let converters = load_converters(&launcher_root)?;
        Ok(Self {
            launcher_root,
            converters,
        })
    }

    fn resolve_under_root(&self, rel_path: &str) -> Option<PathBuf> {
        let mut normalized = PathBuf::new();
        for component in Path::new(rel_path).components() {
            match component {
                Component::Normal(value) => normalized.push(value),
                Component::CurDir => {}
                _ => return None,
            }
        }
        Some(self.launcher_root.join(normalized))
    }

    fn write_under_root(&self, rel_path: &str, content: &[u8]) -> Result<()> {
        let target = self
            .resolve_under_root(rel_path)
            .ok_or_else(|| anyhow!("path '{}' escapes launcher root", rel_path))?;
        if let Some(parent) = target.parent() {
            wasmtime::error::Context::with_context(fs::create_dir_all(parent), || {
                format!("failed to create target directory {}", parent.display())
            })?;
        }
        wasmtime::error::Context::with_context(fs::write(&target, content), || {
            format!("failed writing target file {}", target.display())
        })?;
        Ok(())
    }

    fn copy_assets_from_mod(&self, mod_path: &Path, ignore_patterns: &[String]) -> Result<()> {
        let assets_root = mod_path.join("assets");
        let assets_archive = mod_path.join("assets.tar.zstd");
        if !assets_root.exists() && !assets_archive.exists() {
            return Ok(());
        }

        let ignore_matchers = build_matchers(ignore_patterns)?;
        let mut selected_assets: Vec<SelectedAsset> = Vec::new();
        for file in collect_files(&assets_root)? {
            let rel = Context::with_context(file.strip_prefix(&assets_root), || {
                format!("failed to strip assets prefix for {}", file.display())
            })?;
            let rel_unix = rel.to_string_lossy().replace('\\', "/");
            if ignore_matchers
                .iter()
                .any(|matcher| matcher.is_match(rel_unix.as_str()))
            {
                continue;
            }
            let data = Context::with_context(fs::read(&file), || {
                format!("failed to read mod asset {}", file.display())
            })?;
            selected_assets.push(SelectedAsset {
                rel_unix,
                source_hash_hex: blake3::hash(&data).to_hex().to_string(),
                mtime: file_mtime_secs(&file)?,
                data,
            });
        }

        if assets_archive.exists() {
            for entry in read_zstd_tar_entries(&assets_archive)? {
                if ignore_matchers
                    .iter()
                    .any(|matcher| matcher.is_match(entry.rel_unix.as_str()))
                {
                    continue;
                }
                selected_assets.push(entry);
            }
        }

        let converted_assets = self.prepare_converted_assets(mod_path, &selected_assets)?;

        for asset in selected_assets {
            self.write_under_root(&asset.rel_unix, &asset.data)?;
        }

        for (output_rel, output_data) in converted_assets {
            self.write_under_root(&output_rel, &output_data)?;
        }

        Ok(())
    }

    fn prepare_converted_assets(
        &self,
        mod_path: &Path,
        selected_assets: &[SelectedAsset],
    ) -> Result<HashMap<String, Vec<u8>>> {
        if self.converters.is_empty() {
            return Ok(HashMap::new());
        }

        let mut cache = ConvertCache::load(mod_path.join("converts.tar.zstd"))?;
        let mut outputs = HashMap::new();

        for asset in selected_assets {
            let source_parent = Path::new(&asset.rel_unix)
                .parent()
                .map(|path| path.to_string_lossy().replace('\\', "/"))
                .unwrap_or_default();

            for converter in &self.converters {
                if !converter.matcher.is_match(asset.rel_unix.as_str()) {
                    continue;
                }

                let cache_key = format!("{}::{}", converter.id, asset.rel_unix);
                if let Some(cache_entry) = cache.manifest.entries.get(&cache_key)
                    && cache_entry.source_hash_hex == asset.source_hash_hex
                    && let Some(cached) = cache.files.get(&cache_entry.output_rel)
                {
                    outputs.insert(cache_entry.output_rel.clone(), cached.data.clone());
                    continue;
                }

                let (output_name, output_data) = anyhow::Context::with_context(
                    run_converter(&converter.wasm_path, &asset.rel_unix, &asset.data),
                    || {
                        format!(
                            "converter {} failed for {}",
                            converter.wasm_path.display(),
                            asset.rel_unix
                        )
                    },
                )?;

                let output_rel = if source_parent.is_empty() {
                    output_name
                } else {
                    format!("{source_parent}/{output_name}")
                };

                outputs.insert(output_rel.clone(), output_data.clone());
                cache.manifest.entries.insert(
                    cache_key,
                    ConvertManifestEntry {
                        output_rel: output_rel.clone(),
                        source_hash_hex: asset.source_hash_hex.clone(),
                    },
                );
                cache.files.insert(
                    output_rel.clone(),
                    CachedConverted {
                        mtime: asset.mtime,
                        data: output_data,
                    },
                );
                cache.files.insert(
                    format!("{output_rel}.blake3"),
                    CachedConverted {
                        mtime: asset.mtime,
                        data: asset.source_hash_hex.as_bytes().to_vec(),
                    },
                );
                cache.dirty = true;
            }
        }

        cache.save_if_dirty()?;
        Ok(outputs)
    }
}

#[derive(Default, Serialize, Deserialize)]
struct ConvertManifest {
    entries: HashMap<String, ConvertManifestEntry>,
}

#[derive(Clone, Serialize, Deserialize)]
struct ConvertManifestEntry {
    output_rel: String,
    source_hash_hex: String,
}

struct CachedConverted {
    mtime: u64,
    data: Vec<u8>,
}

struct ConvertCache {
    path: PathBuf,
    manifest: ConvertManifest,
    files: HashMap<String, CachedConverted>,
    dirty: bool,
}

impl ConvertCache {
    fn load(path: PathBuf) -> Result<Self> {
        if !path.exists() {
            return Ok(Self {
                path,
                manifest: ConvertManifest::default(),
                files: HashMap::new(),
                dirty: false,
            });
        }

        let file = wasmtime::error::Context::with_context(fs::File::open(&path), || {
            format!("failed to open convert cache {}", path.display())
        })?;
        let mut decoder =
            wasmtime::error::Context::with_context(zstd::stream::read::Decoder::new(file), || {
                format!("failed to decode convert cache {}", path.display())
            })?;
        let mut decoded = Vec::new();
        wasmtime::error::Context::with_context(decoder.read_to_end(&mut decoded), || {
            format!("failed to read convert cache payload {}", path.display())
        })?;

        let mut archive = tar::Archive::new(Cursor::new(decoded));
        let mut manifest = ConvertManifest::default();
        let mut files = HashMap::new();

        for entry in wasmtime::error::Context::context(
            archive.entries(),
            "failed to iterate convert cache archive entries",
        )? {
            let mut entry = wasmtime::error::Context::context(
                entry,
                "failed to read convert cache archive entry",
            )?;
            let path_key = wasmtime::error::Context::context(
                entry.path(),
                "failed reading convert cache entry path",
            )?
            .to_string_lossy()
            .replace('\\', "/");
            let mtime = entry.header().mtime().unwrap_or(0);
            let mut content = Vec::new();
            wasmtime::error::Context::context(
                entry.read_to_end(&mut content),
                "failed reading convert cache entry bytes",
            )?;

            if path_key == "__manifest.json" {
                manifest = wasmtime::error::Context::context(
                    serde_json::from_slice(&content),
                    "failed parsing convert cache manifest",
                )?;
                continue;
            }

            files.insert(
                path_key,
                CachedConverted {
                    mtime,
                    data: content,
                },
            );
        }

        Ok(Self {
            path,
            manifest,
            files,
            dirty: false,
        })
    }

    fn save_if_dirty(&self) -> Result<()> {
        if !self.dirty {
            return Ok(());
        }

        if let Some(parent) = self.path.parent() {
            fs::create_dir_all(parent)?;
        }

        let mut tar_payload = Vec::new();
        {
            let mut builder = tar::Builder::new(&mut tar_payload);

            let manifest_bytes = wasmtime::error::Context::context(
                serde_json::to_vec(&self.manifest),
                "failed serializing convert cache manifest",
            )?;
            let mut manifest_header = tar::Header::new_gnu();
            manifest_header.set_size(manifest_bytes.len() as u64);
            manifest_header.set_mode(0o644);
            manifest_header.set_mtime(now_secs());
            manifest_header.set_cksum();
            builder.append_data(
                &mut manifest_header,
                "__manifest.json",
                Cursor::new(manifest_bytes),
            )?;

            let mut keys: Vec<&String> = self.files.keys().collect();
            keys.sort();
            for key in keys {
                let entry = self
                    .files
                    .get(key)
                    .ok_or_else(|| anyhow!("missing convert cache entry for key {key}"))?;
                let mut header = tar::Header::new_gnu();
                header.set_size(entry.data.len() as u64);
                header.set_mode(0o644);
                header.set_mtime(entry.mtime);
                header.set_cksum();
                builder.append_data(&mut header, key, Cursor::new(entry.data.as_slice()))?;
            }
            builder.finish()?;
        }

        let encoded = wasmtime::error::Context::context(
            zstd::encode_all(Cursor::new(tar_payload), 0),
            "failed to encode convert cache",
        )?;
        wasmtime::error::Context::with_context(fs::write(&self.path, encoded), || {
            format!("failed writing convert cache {}", self.path.display())
        })?;

        Ok(())
    }
}

static REGISTRY: OnceCell<Arc<Mutex<OverlayRegistry>>> = OnceCell::new();

pub fn init_overlay_registry(launcher_root: &Path) -> Result<()> {
    let registry = OverlayRegistry::new(launcher_root.canonicalize()?)?;
    let _ = REGISTRY.set(Arc::new(Mutex::new(registry)));
    Ok(())
}

pub fn register_mod_assets(mod_path: &Path, ignore: &[String]) -> Result<()> {
    if let Some(state) = REGISTRY.get() {
        state
            .lock()
            .map_err(|_| anyhow!("overlay registry mutex poisoned"))?
            .copy_assets_from_mod(mod_path, ignore)?;
    }
    Ok(())
}

pub fn read_global(path: &str) -> Option<String> {
    let state = REGISTRY.get()?.lock().ok()?;
    let file = state.resolve_under_root(path)?;
    fs::read_to_string(file).ok()
}

pub fn write_global(path: String, content: Vec<u8>, append: bool) -> Result<()> {
    if let Some(state) = REGISTRY.get() {
        let state = state
            .lock()
            .map_err(|_| anyhow!("overlay registry mutex poisoned"))?;
        let Some(file_path) = state.resolve_under_root(&path) else {
            return Err(anyhow!("path '{}' escapes launcher root", path));
        };

        if let Some(parent) = file_path.parent() {
            fs::create_dir_all(parent)?;
        }

        if append && file_path.exists() {
            let mut existing =
                wasmtime::error::Context::with_context(fs::read(&file_path), || {
                    format!(
                        "failed to read existing global file {}",
                        file_path.display()
                    )
                })?;
            existing.extend_from_slice(&content);
            fs::write(&file_path, existing)?;
        } else {
            fs::write(&file_path, content)?;
        }
    }
    Ok(())
}

pub fn mount_process_local_overlay() -> Result<()> {
    Ok(())
}

fn collect_files(root: &Path) -> Result<Vec<PathBuf>> {
    let mut files = Vec::new();
    if !root.exists() {
        return Ok(files);
    }

    let mut stack = vec![root.to_path_buf()];
    while let Some(path) = stack.pop() {
        for entry in wasmtime::error::Context::with_context(fs::read_dir(&path), || {
            format!("failed to read directory {}", path.display())
        })? {
            let entry = entry?;
            let p = entry.path();
            if p.is_dir() {
                stack.push(p);
            } else if p.is_file() {
                files.push(p);
            }
        }
    }

    Ok(files)
}

fn build_matchers(patterns: &[String]) -> Result<Vec<GlobMatcher>> {
    patterns
        .iter()
        .map(|pattern| Glob::new(pattern).map(|glob| glob.compile_matcher()))
        .collect::<std::result::Result<Vec<_>, _>>()
        .map_err(Into::into)
}

fn read_zstd_tar_entries(archive_path: &Path) -> Result<Vec<SelectedAsset>> {
    let file = Context::with_context(fs::File::open(archive_path), || {
        format!("failed to open asset archive {}", archive_path.display())
    })?;
    let mut decoder = Context::with_context(zstd::stream::read::Decoder::new(file), || {
        format!("failed to decode asset archive {}", archive_path.display())
    })?;
    let mut decoded = Vec::new();
    Context::with_context(decoder.read_to_end(&mut decoded), || {
        format!("failed to read asset archive {}", archive_path.display())
    })?;

    let mut archive = tar::Archive::new(Cursor::new(decoded));
    let mut entries = Vec::new();
    for entry in Context::context(
        archive.entries(),
        "failed to iterate entries in assets.tar.zstd",
    )? {
        let mut entry = Context::context(entry, "failed reading assets archive entry")?;
        if !entry.header().entry_type().is_file() {
            continue;
        }

        let rel_unix = Context::context(entry.path(), "failed reading assets archive path")?
            .to_string_lossy()
            .replace('\\', "/");
        if rel_unix.is_empty() {
            continue;
        }

        let mut data = Vec::new();
        Context::context(
            entry.read_to_end(&mut data),
            "failed reading asset archive entry bytes",
        )?;
        entries.push(SelectedAsset {
            rel_unix,
            source_hash_hex: blake3::hash(&data).to_hex().to_string(),
            mtime: entry.header().mtime().unwrap_or(0),
            data,
        });
    }

    Ok(entries)
}

fn load_converters(launcher_root: &Path) -> Result<Vec<ConverterEntry>> {
    let converts_dir = launcher_root.join("converts");
    if !converts_dir.exists() {
        return Ok(Vec::new());
    }

    let converter_files = collect_files(&converts_dir)?;
    let mut converters = Vec::new();
    for wasm_path in converter_files
        .into_iter()
        .filter(|path| path.extension().and_then(|ext| ext.to_str()) == Some("wasm"))
    {
        let relative =
            wasmtime::error::Context::with_context(wasm_path.strip_prefix(&converts_dir), || {
                format!(
                    "failed to strip converts directory prefix for {}",
                    wasm_path.display()
                )
            })?;
        let Some(parent) = relative.parent() else {
            continue;
        };
        let pattern = parent.to_string_lossy().replace('\\', "/");
        if pattern.is_empty() || pattern == "." {
            continue;
        }

        let id = relative.to_string_lossy().replace('\\', "/");
        let matcher = wasmtime::error::Context::with_context(Glob::new(&pattern), || {
            format!("invalid converter glob pattern '{}'", pattern)
        })?
        .compile_matcher();
        converters.push(ConverterEntry {
            id,
            matcher,
            wasm_path,
        });
    }

    Ok(converters)
}

fn run_converter(
    wasm_path: &Path,
    source_name: &str,
    source_bytes: &[u8],
) -> Result<(String, Vec<u8>)> {
    let wasm = wasmtime::error::Context::with_context(fs::read(wasm_path), || {
        format!("failed to read converter wasm {}", wasm_path.display())
    })?;

    let engine = Engine::default();
    let module =
        wasmtime::error::Context::with_context(Module::from_binary(&engine, &wasm), || {
            format!("failed to compile converter module {}", wasm_path.display())
        })?;
    let mut store = Store::new(&engine, ());
    let instance =
        wasmtime::error::Context::with_context(Instance::new(&mut store, &module, &[]), || {
            format!(
                "failed to instantiate converter module {}",
                wasm_path.display()
            )
        })?;

    let memory = wasmtime::error::Context::context(
        instance.get_memory(&mut store, "memory"),
        "converter module must export memory",
    )?;

    let alloc: TypedFunc<i32, i32> = wasmtime::error::Context::context(
        instance.get_typed_func(&mut store, "alloc"),
        "converter module must export alloc(size: i32) -> i32",
    )?;
    let dealloc: TypedFunc<(i32, i32), ()> = wasmtime::error::Context::context(
        instance.get_typed_func(&mut store, "dealloc"),
        "converter module must export dealloc(ptr: i32, len: i32)",
    )?;

    let file_name_c = wasmtime::error::Context::context(
        CString::new(source_name),
        "source file name contains null byte",
    )?;
    let file_name_bytes = file_name_c.as_bytes_with_nul();

    let name_ptr = alloc.call(&mut store, file_name_bytes.len() as i32)?;
    write_memory(&memory, &mut store, name_ptr, file_name_bytes)?;

    let data_ptr = alloc.call(&mut store, source_bytes.len() as i32)?;
    write_memory(&memory, &mut store, data_ptr, source_bytes)?;

    let packed = if let Ok(convert64) =
        instance.get_typed_func::<(i32, i32, i32), i64>(&mut store, "convert")
    {
        convert64.call(&mut store, (name_ptr, data_ptr, source_bytes.len() as i32))? as u64
    } else {
        let convert32: TypedFunc<(i32, i32, i32), i32> = wasmtime::error::Context::context(
            instance.get_typed_func(&mut store, "convert"),
            "converter module must export convert(file_name, data, size)",
        )?;
        convert32.call(&mut store, (name_ptr, data_ptr, source_bytes.len() as i32))? as u64
    };

    let out_ptr = (packed >> 32) as i32;
    let out_len = (packed & 0xffff_ffff) as i32;
    if out_ptr <= 0 || out_len <= 4 {
        return Err(anyhow!("converter returned invalid output buffer"));
    }

    let mut output = vec![0u8; out_len as usize];
    read_memory(&memory, &mut store, out_ptr, &mut output)?;

    dealloc.call(&mut store, (name_ptr, file_name_bytes.len() as i32))?;
    dealloc.call(&mut store, (data_ptr, source_bytes.len() as i32))?;
    dealloc.call(&mut store, (out_ptr, out_len))?;

    let name_len = u32::from_le_bytes([output[0], output[1], output[2], output[3]]) as usize;
    if output.len() < 4 + name_len {
        return Err(anyhow!("converter output header is truncated"));
    }

    let file_name = wasmtime::error::Context::context(
        String::from_utf8(output[4..4 + name_len].to_vec()),
        "converter returned invalid utf-8 file name",
    )?;
    let file_data = output[4 + name_len..].to_vec();
    Ok((file_name, file_data))
}

fn write_memory(memory: &Memory, store: &mut Store<()>, offset: i32, data: &[u8]) -> Result<()> {
    memory
        .write(store, offset as usize, data)
        .map_err(|err| anyhow!("failed writing wasm memory: {err}"))
}

fn read_memory(memory: &Memory, store: &mut Store<()>, offset: i32, out: &mut [u8]) -> Result<()> {
    memory
        .read(store, offset as usize, out)
        .map_err(|err| anyhow!("failed reading wasm memory: {err}"))
}

fn file_mtime_secs(path: &Path) -> Result<u64> {
    let modified = wasmtime::error::Context::with_context(
        wasmtime::error::Context::with_context(fs::metadata(path), || {
            format!("failed reading metadata for {}", path.display())
        })?
        .modified(),
        || format!("failed reading mtime for {}", path.display()),
    )?;
    Ok(modified
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs())
}

fn now_secs() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_secs()
}
