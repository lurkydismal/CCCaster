/// Engine logging/memory APIs and script patch parsing helpers.
use crate::api::{make_patch, read_memory, remove_patch, write_memory};
use crate::{
    LOG_DEBUG, LOG_ERROR, LOG_INFO, LOG_TRACE, LOG_WARNING, modloader_debug, modloader_error,
    modloader_info, modloader_trace, modloader_warning,
};
use anyhow::Result;
use mlua::{Lua, Table, Value};

use super::engine_require_dispatch::is_probably_writable;

pub(super) fn install_engine_log_api(lua: &Lua, engine_table: &Table) -> Result<()> {
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

pub(super) fn install_engine_memory_api(lua: &Lua, engine_table: &Table) -> Result<()> {
    let memory_table = lua.create_table()?;

    let read_fn = lua.create_function(|lua, (address, length): (Value, usize)| {
        let addr = match parse_lua_address(address) {
            Ok(value) => value,
            Err(err) => {
                modloader_warning!("Engine.memory.read failed: {err}");
                return Ok(Value::Nil);
            }
        };

        let bytes = match read_memory(addr, length) {
            Some(bytes) => bytes,
            None => {
                modloader_warning!(
                    "Engine.memory.read failed: host denied range read 0x{:X}..+{}",
                    addr,
                    length
                );
                return Ok(Value::Nil);
            }
        };

        let out = lua.create_table()?;
        for (idx, byte) in bytes.iter().copied().enumerate() {
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

        if !write_memory(addr, &parsed) {
            modloader_warning!(
                "Engine.memory.write failed: host denied range write 0x{:X}..+{}",
                addr,
                parsed.len()
            );
            return Ok(false);
        }
        Ok(true)
    })?;

    let patch_make_fn = lua.create_function(|lua, args: mlua::MultiValue| {
        let patches = parse_script_patch_args(args)?;
        if patches.is_empty() {
            modloader_warning!("Engine.memory.patch.make failed: no patch entries provided");
            return Ok(Value::Nil);
        }

        let mut handles: Vec<u32> = Vec::new();
        for patch in patches {
            let end = match patch.address.checked_add(patch.bytes.len()) {
                Some(value) => value,
                None => {
                    modloader_warning!(
                        "Engine.memory.patch.make failed: address overflow at 0x{:X}",
                        patch.address
                    );
                    return Ok(Value::Nil);
                }
            };

            if !is_probably_writable(patch.address, patch.bytes.len()) {
                modloader_warning!(
                    "Engine.memory.patch.make failed: unwritable range 0x{:X}..0x{:X}",
                    patch.address,
                    end
                );
                return Ok(Value::Nil);
            }

            let handle = make_patch(patch.address, &patch.bytes);
            handles.push(handle);
        }

        if handles.len() == 1 {
            // NOTE: handle values are 32-bit for game compatibility.
            return Ok(Value::Integer(handles[0] as i32));
        }

        let out = lua.create_table()?;
        for (idx, handle) in handles.iter().enumerate() {
            out.set(idx + 1, *handle)?;
        }
        Ok(Value::Table(out))
    })?;

    let patch_remove_fn = lua.create_function(|_, id: u32| Ok(remove_patch(id)))?;

    memory_table.set("read", read_fn)?;
    memory_table.set("write", write_fn)?;
    let patch_table = lua.create_table()?;
    patch_table.set("make", patch_make_fn)?;
    patch_table.set("remove", patch_remove_fn)?;
    memory_table.set("patch", patch_table)?;
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
                "Engine.memory.patch.make expects (address, bytes[, pattern]) groups",
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
                    "Engine.memory.patch.make table entries must be patch objects",
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
        let found = read_memory(address + idx, 1)
            .and_then(|bytes| bytes.first().copied())
            .ok_or_else(|| {
                format!(
                    "pattern check failed at 0x{:X}: address is not readable",
                    address + idx
                )
            })?;
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
