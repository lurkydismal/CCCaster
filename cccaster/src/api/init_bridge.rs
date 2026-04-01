use std::{fs, os::raw::c_char};

use crate::{
    loader, main, modloader_debug, modloader_error, modloader_info, modloader_trace,
    modloader_warning, parse_args,
};

use super::Api;

#[ctor::dtor]
fn cccaster_dtor() {
    modloader_info!("cccaster dtor invoked; shutting down addons");
    loader::shutdown_before_unload();
}

fn parse_init_json(json: *const c_char, json_len: usize) -> bool {
    if json.is_null() {
        modloader_error!("init received null json pointer");
        return false;
    }

    // SAFETY: host provides the pointer and explicit length in `init`.
    let slice = unsafe { std::slice::from_raw_parts(json as *const u8, json_len) };
    modloader_trace!("Received init JSON payload with {} bytes", json_len);

    match std::str::from_utf8(slice) {
        Ok(json_str) => {
            modloader_debug!(
                "init JSON payload validated as UTF-8 ({} chars)",
                json_str.len()
            );
            let preview = json_str.chars().take(120).collect::<String>();
            modloader_trace!("init JSON preview (first 120 chars): {}", preview);
            if let Err(err) = parse_args(json_str) {
                modloader_error!("Failed to parse modloader init arguments JSON: {err}");
                return false;
            }
            true
        }
        Err(err) => {
            modloader_warning!("init JSON payload is not valid UTF-8: {}", err);
            false
        }
    }
}

fn run_main() {
    modloader_info!("Calling async modloader main entrypoint");

    if let Err(err) = main() {
        modloader_error!("Modloader startup failed: {}", err);

        for (i, cause) in err.chain().skip(1).enumerate() {
            modloader_error!("Root cause [{}]: {}", i, cause);
        }
    } else {
        modloader_info!("Modloader main entrypoint completed successfully");
    }
}

fn has_multiple_threads() -> std::io::Result<bool> {
    let s = fs::read_to_string("/proc/self/status")?;
    for line in s.lines() {
        if let Some(rest) = line.strip_prefix("Threads:") {
            let n: usize = rest.trim().parse().unwrap_or(1);
            return Ok(n > 1);
        }
    }
    Ok(false)
}

#[unsafe(no_mangle)]
pub extern "C" fn init(vtable: *const Api, json: *const c_char, json_len: usize) -> bool {
    modloader_info!("Initializing C API bridge");

    if !super::set_api_from_ptr(vtable) {
        modloader_error!("init received null vtable pointer");
        return false;
    }
    modloader_debug!("Registered API vtable function pointers");

    if !parse_init_json(json, json_len) {
        return false;
    }

    if has_multiple_threads().unwrap_or(true) {
        modloader_error!("Failed to initialize process-local overlay: not single-threaded");
        return false;
    }

    if let Err(err) = loader::init_process_local_overlay() {
        modloader_error!("Failed to initialize process-local overlay: {}", err);
        return false;
    }

    run_main();
    true
}
