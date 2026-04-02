use crate::api::{Handle, create_detour, remove_detour};
use crate::loader;
use crate::{modloader_debug, modloader_error, modloader_warning};
use anyhow::{Result, anyhow};
use once_cell::sync::Lazy;
use serde_json::json;
use std::collections::HashMap;
use std::sync::Mutex;

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct HookRegisters {
    pub edi: u32,
    pub esi: u32,
    pub ebp: u32,
    pub esp_at_pushad: u32,
    pub ebx: u32,
    pub edx: u32,
    pub ecx: u32,
    pub eax: u32,
    pub eflags: u32,
}

#[repr(C)]
struct HookFrame {
    regs: HookRegisters,
    ret_addr: u32,
}

#[derive(Clone)]
struct HookMetadata {
    event_name: String,
    replaced_bytes: Vec<u8>,
    trampoline_addr: usize,
    detour_handle: Handle,
}

static HOOKS: Lazy<Mutex<HashMap<usize, HookMetadata>>> = Lazy::new(|| Mutex::new(HashMap::new()));

pub fn register_trampoline_hook(
    call_site: usize,
    event_name: String,
    replaced_bytes: Vec<u8>,
) -> Result<()> {
    let (detour_handle, trampoline_addr) =
        create_detour(call_site, hook_entry as *const () as usize, true)
            .ok_or_else(|| anyhow!("failed to create detour at 0x{:X}", call_site))?;

    HOOKS
        .lock()
        .map_err(|_| anyhow!("hook registry mutex poisoned"))?
        .insert(
            call_site,
            HookMetadata {
                event_name,
                replaced_bytes,
                trampoline_addr,
                detour_handle,
            },
        );

    Ok(())
}

pub fn unregister_trampoline_hook(call_site: usize) {
    let removed = HOOKS
        .lock()
        .ok()
        .and_then(|mut guard| guard.remove(&call_site));
    if let Some(metadata) = removed {
        let _ = remove_detour(metadata.detour_handle);
    }
}

pub fn execute_replaced_bytes(call_site: usize) -> Result<bool> {
    let trampoline = HOOKS
        .lock()
        .map_err(|_| anyhow!("hook registry mutex poisoned"))?
        .get(&call_site)
        .map(|meta| meta.trampoline_addr)
        .ok_or_else(|| anyhow!("unknown hook call site 0x{:X}", call_site))?;

    let f: extern "C" fn() = unsafe { std::mem::transmute(trampoline) };
    f();
    Ok(true)
}

#[cfg(target_arch = "x86")]
#[unsafe(naked)]
pub unsafe extern "C" fn hook_entry() {
    std::arch::naked_asm!(
        "sub esp, 4",
        "pushfd",
        "pushad",
        "mov eax, esp",
        "push eax",
        "call {dispatch}",
        "add esp, 4",
        "mov [esp + 36], eax",
        "popad",
        "popfd",
        "mov eax, [esp]",
        "add esp, 4",
        "test eax, eax",
        "jz 2f",
        "jmp eax",
        "2:",
        "ret",
        dispatch = sym hook_dispatch,
    );
}

#[cfg(not(target_arch = "x86"))]
pub unsafe extern "C" fn hook_entry() {}

extern "C" fn hook_dispatch(frame_ptr: *const HookFrame) -> usize {
    if frame_ptr.is_null() {
        return 0;
    }
    let frame = unsafe { &*frame_ptr };
    let ret = frame.ret_addr as usize;

    let Some(call_site) = (unsafe { find_call_site(ret as *const u8) }).map(|p| p as usize) else {
        return 0;
    };

    let metadata = match HOOKS.lock() {
        Ok(guard) => guard.get(&call_site).cloned(),
        Err(_) => None,
    };
    let Some(metadata) = metadata else {
        return 0;
    };

    let payload = json!({
        "call_site": call_site,
        "return_address": ret,
        "registers": {
            "eax": frame.regs.eax,
            "ebx": frame.regs.ebx,
            "ecx": frame.regs.ecx,
            "edx": frame.regs.edx,
            "esi": frame.regs.esi,
            "edi": frame.regs.edi,
            "ebp": frame.regs.ebp,
            "esp": frame.regs.esp_at_pushad,
            "eflags": frame.regs.eflags
        },
        "replaced_bytes": metadata.replaced_bytes.iter().map(|b| *b as u32).collect::<Vec<u32>>()
    });

    if let Err(err) = loader::dispatch_engine_event(
        metadata.event_name,
        vec![payload.to_string()],
        vec!["json".to_string()],
    ) {
        modloader_warning!("hook_dispatch failed to enqueue hook event: {}", err);
    } else {
        modloader_debug!(
            "hook_dispatch queued scripting hook event at 0x{:X}",
            call_site
        );
    }

    metadata.trampoline_addr
}

unsafe fn find_call_site(ret: *const u8) -> Option<*const u8> {
    for i in 1..=15 {
        let p = unsafe { ret.sub(i) };
        if unsafe { *p } == 0xE8 {
            return Some(p);
        }
        if unsafe { *p } == 0xFF {
            let modrm = unsafe { *p.add(1) };
            let reg = (modrm >> 3) & 0b111;
            let mode = modrm >> 6;
            if reg == 2 && mode != 0b11 {
                return Some(p);
            }
        }
    }
    modloader_error!(
        "failed to locate call instruction near return address {:p}",
        ret
    );
    None
}
