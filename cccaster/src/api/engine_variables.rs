use std::os::raw::c_char;

use serde_json::Value as JsonValue;

use crate::{loader, modloader_error, modloader_info};

fn parse_name(name: *const c_char, name_len: usize) -> Option<String> {
    if name.is_null() {
        modloader_error!("register_engine_variable_json received null name pointer");
        return None;
    }

    // SAFETY: host supplies pointer and explicit length.
    let name_slice = unsafe { std::slice::from_raw_parts(name as *const u8, name_len) };
    let parsed = match std::str::from_utf8(name_slice) {
        Ok(name) => name.trim(),
        Err(err) => {
            modloader_error!(
                "register_engine_variable_json received invalid UTF-8 name: {}",
                err
            );
            return None;
        }
    };

    if parsed.is_empty() {
        modloader_error!("register_engine_variable_json received empty name");
        return None;
    }

    Some(parsed.to_owned())
}

fn parse_value(name: &str, value_json: *const c_char, value_json_len: usize) -> Option<JsonValue> {
    if value_json.is_null() {
        return Some(JsonValue::Null);
    }

    // SAFETY: host supplies pointer and explicit length.
    let value_slice =
        unsafe { std::slice::from_raw_parts(value_json as *const u8, value_json_len) };
    let value_str = match std::str::from_utf8(value_slice) {
        Ok(value) => value,
        Err(err) => {
            modloader_error!(
                "register_engine_variable_json received invalid UTF-8 JSON payload for '{}': {}",
                name,
                err
            );
            return None;
        }
    };

    match serde_json::from_str::<JsonValue>(value_str) {
        Ok(value) => Some(value),
        Err(err) => {
            modloader_error!(
                "register_engine_variable_json failed to parse JSON payload for '{}': {}",
                name,
                err
            );
            None
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn register_engine_variable_json(
    name: *const c_char,
    name_len: usize,
    value_json: *const c_char,
    value_json_len: usize,
) -> bool {
    let Some(name) = parse_name(name, name_len) else {
        return false;
    };

    let Some(parsed_value) = parse_value(&name, value_json, value_json_len) else {
        return false;
    };

    match loader::register_engine_variable(name.clone(), parsed_value) {
        Ok(()) => {
            modloader_info!("Registered external Engine override for key '{}'", name);
            true
        }
        Err(err) => {
            modloader_error!(
                "Failed to register external Engine override for key '{}': {}",
                name,
                err
            );
            false
        }
    }
}
