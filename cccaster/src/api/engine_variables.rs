use std::{ffi::CStr, os::raw::c_char};

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

fn parse_c_string_array(
    data: *const *const c_char,
    amount: usize,
    label: &str,
) -> Option<Vec<String>> {
    if amount == 0 {
        return Some(Vec::new());
    }
    if data.is_null() {
        modloader_error!(
            "call_engine_event_with_args received null {} pointer with non-zero amount={}",
            label,
            amount
        );
        return None;
    }

    let mut parsed = Vec::with_capacity(amount);
    for index in 0..amount {
        // SAFETY: `data` points to an array with `amount` entries, provided by the host.
        let item_ptr = unsafe { *data.add(index) };
        if item_ptr.is_null() {
            modloader_error!(
                "call_engine_event_with_args received null {} pointer at index {}",
                label,
                index
            );
            return None;
        }

        // SAFETY: `item_ptr` is expected to reference a valid null-terminated C string.
        let value = match unsafe { CStr::from_ptr(item_ptr) }.to_str() {
            Ok(value) => value.to_owned(),
            Err(err) => {
                modloader_error!(
                    "call_engine_event_with_args received invalid UTF-8 in {} at index {}: {}",
                    label,
                    index,
                    err
                );
                return None;
            }
        };
        parsed.push(value);
    }

    Some(parsed)
}

#[unsafe(no_mangle)]
pub extern "C" fn call_engine_event_with_args(
    event_name: *const c_char,
    event_name_len: usize,
    args: *const *const c_char,
    arg_types: *const *const c_char,
    args_amount: usize,
) -> bool {
    let Some(event_name) = parse_name(event_name, event_name_len) else {
        modloader_error!("call_engine_event_with_args received invalid event name");
        return false;
    };

    let Some(arguments) = parse_c_string_array(args, args_amount, "args") else {
        return false;
    };
    let Some(argument_types) = parse_c_string_array(arg_types, args_amount, "arg_types") else {
        return false;
    };

    match loader::dispatch_engine_event(event_name.clone(), arguments, argument_types) {
        Ok(()) => {
            modloader_info!("Queued external Engine event dispatch for '{}'", event_name);
            true
        }
        Err(err) => {
            modloader_error!(
                "Failed to queue external Engine event dispatch for '{}': {}",
                event_name,
                err
            );
            false
        }
    }
}
