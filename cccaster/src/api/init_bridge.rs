use std::os::raw::c_char;

use dtor::dtor;

use crate::{
    loader, main, modloader_debug, modloader_error, modloader_info, modloader_trace,
    modloader_warning, parse_args,
};

use super::Api;

#[dtor(unsafe)]
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

    run_main();
    true
}
