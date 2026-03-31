/// Engine dispatch/require integration and platform memory accessibility checks.
use crate::modloader_trace;
use crate::types::ModMeta;
use anyhow::Result;
use mlua::{Function, Lua, Table, Value};
use std::collections::HashSet;
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};

use super::path_utils::{normalize_path, sanitize_identifier};

#[cfg(unix)]
pub(super) fn is_probably_readable(address: usize, length: usize) -> bool {
    is_probably_accessible_unix(address, length, libc::PROT_READ)
}

#[cfg(unix)]
pub(super) fn is_probably_writable(address: usize, length: usize) -> bool {
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

pub(super) fn install_engine_dispatch_api(
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
pub(super) fn register_engine_require(
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
