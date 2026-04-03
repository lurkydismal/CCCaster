use crate::api::{Handle, create_detour, read_memory, remove_detour};
use crate::loader;
use crate::{modloader_debug, modloader_info, modloader_trace, modloader_warning};
use anyhow::{Result, anyhow};
use once_cell::sync::Lazy;
use serde_json::json;
use std::collections::HashMap;
use std::sync::Mutex;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct HookRegisters {
    pub edi: u32,
    pub esi: u32,
    pub ebp: u32,
    pub esp_at_pushad: u32,
    pub ebx: u32,
    pub edx: u32,
    pub ecx: u32,
    pub eax: u32,
}

#[derive(Clone, Copy, Debug, Default)]
pub struct HookRegisterOverrides {
    pub edi: Option<u32>,
    pub esi: Option<u32>,
    pub ebp: Option<u32>,
    pub esp_at_pushad: Option<u32>,
    pub ebx: Option<u32>,
    pub edx: Option<u32>,
    pub ecx: Option<u32>,
    pub eax: Option<u32>,
}

#[repr(C)]
struct HookFrame {
    regs: HookRegisters,
    eflags: u32,
    dispatch_result: u32,
    ret_addr: u32,
    pushed_addr: u32,
}

#[derive(Clone, Copy, Debug, Default)]
pub struct HookDecision {
    pub registers: Option<HookRegisterOverrides>,
    pub run_trampoline: bool,
}

#[derive(Clone)]
struct HookMetadata {
    event_name: String,
    replaced_bytes: Vec<u8>,
    trampoline_bytes: Vec<u8>,
    trampoline_addr: usize,
    detour_handle: Handle,
}

static HOOKS: Lazy<Mutex<HashMap<usize, HookMetadata>>> = Lazy::new(|| Mutex::new(HashMap::new()));

pub fn register_trampoline_hook(
    call_site: usize,
    event_name: String,
    replaced_bytes: Vec<u8>,
    suspend_process: bool,
) -> Result<()> {
    modloader_info!(
        "Registering trampoline hook: call_site=0x{:X}, event='{}', replaced_len={}",
        call_site,
        event_name,
        replaced_bytes.len()
    );
    let (detour_handle, trampoline_addr) =
        create_detour(call_site, hook_entry as *const () as usize, suspend_process)
            .ok_or_else(|| anyhow!("failed to create detour at 0x{:X}", call_site))?;
    modloader_debug!(
        "create_detour succeeded: call_site=0x{:X}, trampoline=0x{:X}, handle={}",
        call_site,
        trampoline_addr,
        detour_handle
    );

    let mut hooks = HOOKS
        .lock()
        .map_err(|_| anyhow!("hook registry mutex poisoned"))?;
    let trampoline_bytes =
        read_memory(trampoline_addr, replaced_bytes.len() + 5).unwrap_or_default();

    hooks.insert(
        call_site,
        HookMetadata {
            event_name,
            replaced_bytes,
            trampoline_bytes,
            trampoline_addr,
            detour_handle,
        },
    );
    modloader_info!(
        "Hook registry now tracks {} trampoline hooks after inserting 0x{:X}",
        hooks.len(),
        call_site
    );

    Ok(())
}

pub fn unregister_trampoline_hook(call_site: usize) {
    modloader_debug!(
        "Attempting to unregister trampoline hook at 0x{:X}",
        call_site
    );
    let removed = HOOKS
        .lock()
        .ok()
        .and_then(|mut guard| guard.remove(&call_site));
    if let Some(metadata) = removed {
        modloader_trace!(
            "Found hook metadata for 0x{:X}; removing detour handle={}",
            call_site,
            metadata.detour_handle
        );
        let _ = remove_detour(metadata.detour_handle);
    } else {
        modloader_warning!(
            "Requested hook unregister for 0x{:X}, but no hook metadata existed",
            call_site
        );
    }
}

pub fn execute_replaced_bytes(call_site: usize) -> Result<bool> {
    modloader_trace!(
        "Executing replaced bytes via trampoline for 0x{:X}",
        call_site
    );
    let trampoline = HOOKS
        .lock()
        .map_err(|_| anyhow!("hook registry mutex poisoned"))?
        .get(&call_site)
        .map(|meta| meta.trampoline_addr)
        .ok_or_else(|| anyhow!("unknown hook call site 0x{:X}", call_site))?;
    modloader_debug!(
        "Resolved trampoline for 0x{:X} to 0x{:X}; jumping into trampoline",
        call_site,
        trampoline
    );

    let f: extern "C" fn() = unsafe { std::mem::transmute(trampoline) };
    f();
    modloader_trace!("Returned from trampoline execution for 0x{:X}", call_site);
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
        "mov edx, [esp]",
        "add esp, 4",
        "pop ecx",
        "add esp, 4",
        "test edx, edx",
        "jz 2f",
        "jmp edx",
        "2:",
        "jmp ecx",
        dispatch = sym hook_dispatch,
    );
}

#[cfg(not(target_arch = "x86"))]
pub unsafe extern "C" fn hook_entry() {}

extern "C" fn hook_dispatch(frame_ptr: *mut HookFrame) -> usize {
    if frame_ptr.is_null() {
        modloader_warning!("hook_dispatch received null HookFrame pointer");
        return 0;
    }
    let frame = unsafe { &*frame_ptr };
    let ret = frame.ret_addr as usize;
    let call_site = frame.pushed_addr as usize;
    modloader_trace!(
        "hook_dispatch entered: return_address=0x{:X}, eax=0x{:X}, ebx=0x{:X}, ecx=0x{:X}, edx=0x{:X}",
        ret,
        frame.regs.eax,
        frame.regs.ebx,
        frame.regs.ecx,
        frame.regs.edx
    );

    modloader_trace!(
        "hook_dispatch received pushed call-site 0x{:X} for return_address=0x{:X}",
        call_site,
        ret
    );

    let metadata = match HOOKS.lock() {
        Ok(guard) => guard.get(&call_site).cloned(),
        Err(_) => None,
    };
    let Some(metadata) = metadata else {
        modloader_warning!(
            "hook_dispatch found no metadata for call-site 0x{:X}",
            call_site
        );
        return 0;
    };
    modloader_debug!(
        "hook_dispatch dispatching event '{}' for call-site 0x{:X}",
        metadata.event_name,
        call_site
    );

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
            "esp": frame.regs.esp_at_pushad
        },
        "removed_bytes": metadata.replaced_bytes.iter().map(|b| *b as u32).collect::<Vec<u32>>(),
        "trampoline_bytes": metadata.trampoline_bytes.iter().map(|b| *b as u32).collect::<Vec<u32>>()
    });

    match loader::dispatch_hook_event_sync(metadata.event_name, payload.to_string()) {
        Ok(decision) => {
            if let Some(updated) = decision.registers {
                unsafe {
                    if let Some(value) = updated.eax {
                        (*frame_ptr).regs.eax = value;
                    }
                    if let Some(value) = updated.ebx {
                        (*frame_ptr).regs.ebx = value;
                    }
                    if let Some(value) = updated.ecx {
                        (*frame_ptr).regs.ecx = value;
                    }
                    if let Some(value) = updated.edx {
                        (*frame_ptr).regs.edx = value;
                    }
                    if let Some(value) = updated.esi {
                        (*frame_ptr).regs.esi = value;
                    }
                    if let Some(value) = updated.edi {
                        (*frame_ptr).regs.edi = value;
                    }
                    if let Some(value) = updated.ebp {
                        (*frame_ptr).regs.ebp = value;
                    }
                    if let Some(value) = updated.esp_at_pushad {
                        (*frame_ptr).regs.esp_at_pushad = value;
                    }
                }
            }
            if decision.run_trampoline {
                return metadata.trampoline_addr;
            }
        }
        Err(err) => {
            modloader_warning!("hook_dispatch failed to enqueue hook event: {}", err);
        }
    }
    {
        modloader_debug!(
            "hook_dispatch queued scripting hook event at 0x{:X}",
            call_site
        );
    }

    0
}
