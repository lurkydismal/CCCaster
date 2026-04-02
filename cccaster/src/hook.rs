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
        "pushfd",
        "pushad",
        "mov eax, esp",
        "push eax",
        "call {dispatch}",
        "add esp, 4",
        "popad",
        "popfd",
        "ret",
        dispatch = sym hook_dispatch,
    );
}

#[cfg(not(target_arch = "x86"))]
pub unsafe extern "C" fn hook_entry() {}

extern "C" fn hook_dispatch(frame_ptr: *const HookFrame) {
    if frame_ptr.is_null() {
        return;
    }
    let frame = unsafe { &*frame_ptr };
    let ret = frame.ret_addr as usize;

    let Some(call_site) = (unsafe { find_call_site(ret as *const u8) }).map(|p| p as usize) else {
        return;
    };

    let metadata = match HOOKS.lock() {
        Ok(guard) => guard.get(&call_site).cloned(),
        Err(_) => None,
    };
    let Some(metadata) = metadata else {
        return;
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
}

/// Try to recover the address of the instruction that performed the call.
///
/// This scans backward up to 15 bytes from the return address and checks for
/// two common x86 call encodings:
///
/// - `E8 rel32`
///   Direct near call. This is the simplest case and is often exactly 5 bytes.
/// - `FF /2`
///   Indirect near call through a register or memory operand.
///
/// The search is intentionally small because x86 instructions are variable
/// length and 15 bytes is the maximum instruction length on x86.
///
/// Returns:
/// - `Some(call_site)` if a plausible call instruction is found.
/// - `None` if no match is found.
///
/// Safety:
/// - `ret` must point to readable executable memory.
/// - This is only a heuristic; it does not fully decode instructions.
/// - False positives are possible if arbitrary bytes happen to match.
///
/// Important:
/// - For direct calls, `ret - 5` is typically the correct call-site.
/// - The `FF /2` case is broader and less precise without full decoding.
unsafe fn find_call_site(ret: *const u8) -> Option<*const u8> {
    // Scan backward from the return address by a small bounded amount.
    // This is a heuristic search, not a full disassembly.
    for i in 1..=15 {
        let p = unsafe { ret.sub(i) };

        // Direct near call:
        //   E8 xx xx xx xx
        // If we see `E8` at the right offset, we assume this is the call site.
        if unsafe { *p } == 0xE8 {
            return Some(p);
        }

        // Indirect near call:
        //   FF /2
        //
        // `FF` is an opcode group. The ModR/M byte selects the actual operation.
        // For `/2`, the `reg` field in the ModR/M byte must be `2`.
        if unsafe { *p } == 0xFF {
            let modrm = unsafe { *p.add(1) };

            // Bits 5..3 select the /digit for opcode groups.
            let reg = (modrm >> 3) & 0b111;

            // Bits 7..6 select the addressing mode:
            //   00 = memory
            //   01 = memory + 8-bit displacement
            //   10 = memory + 32-bit displacement
            //   11 = register-direct
            //
            // Keeping `mode != 0b11` avoids treating register-direct forms
            // as a memory-style call site match.
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
