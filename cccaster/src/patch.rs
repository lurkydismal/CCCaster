use crate::api::{Handle, make_patch_with_suspend, remove_patch_with_suspend};
use crate::hook;
use crate::{modloader_debug, modloader_info, modloader_trace, modloader_warning};
use serde::Deserialize;
use std::fmt;

use anyhow::{Result, anyhow};

/// Represents a patch entry from patch.json5
#[derive(Deserialize)]
pub struct PatchEntry {
    #[serde(deserialize_with = "deserialize_address")]
    pub address: usize,
    #[serde(default, deserialize_with = "deserialize_hex_bytes")]
    pub bytes: Vec<u8>,
    #[serde(default)]
    pub pattern: Option<String>,
    #[serde(default, alias = "hook_event")]
    pub event: Option<String>,
}

#[derive(Clone)]
pub enum ResolvedPatch {
    Bytes {
        address: usize,
        bytes: Vec<u8>,
    },
    TrampolineHook {
        address: usize,
        overwrite_len: usize,
        replaced_bytes: Vec<u8>,
        event_name: String,
    },
}

impl ResolvedPatch {
    pub fn address(&self) -> usize {
        match self {
            ResolvedPatch::Bytes { address, .. }
            | ResolvedPatch::TrampolineHook { address, .. } => *address,
        }
    }

    pub fn len(&self) -> usize {
        match self {
            ResolvedPatch::Bytes { bytes, .. } => bytes.len(),
            ResolvedPatch::TrampolineHook { overwrite_len, .. } => *overwrite_len,
        }
    }
}

#[derive(Clone, Copy)]
pub struct PatchSpan {
    pub start: usize,
    pub end_exclusive: usize,
}

impl fmt::Display for PatchSpan {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "0x{:X}-0x{:X}", self.start, self.end_exclusive)
    }
}

pub struct OwnedPatchSpan<'a> {
    pub owner: &'a str,
    pub span: PatchSpan,
}

pub fn resolve_patch_entries(entries: &[PatchEntry], mod_id: &str) -> Result<Vec<ResolvedPatch>> {
    modloader_info!(
        "Resolving {} patch entries for mod '{}'",
        entries.len(),
        mod_id
    );
    let mut resolved = Vec::new();

    for (idx, entry) in entries.iter().enumerate() {
        modloader_trace!(
            "Resolving patch[{idx}] for mod '{mod_id}': address=0x{:X}, pattern_present={}, hook_event={:?}, bytes_len={}",
            entry.address,
            entry.pattern.is_some(),
            entry.event,
            entry.bytes.len()
        );
        if let Some(pattern) = entry.pattern.as_deref() {
            if let Some(event_name) = entry.event.as_deref() {
                let hook = resolve_pattern_hook(entry.address, pattern, event_name)?;
                resolved.push(hook);
                modloader_debug!(
                    "Resolved patch[{idx}] as pattern trampoline hook for event '{}'",
                    event_name
                );
                continue;
            }
            let expanded = resolve_pattern_patch(entry.address, pattern, &entry.bytes)
                .map_err(|err| anyhow!("mod {mod_id} patch[{idx}] pattern error: {err}"))?;
            modloader_debug!(
                "Resolved patch[{idx}] pattern into {} byte patch segment(s)",
                expanded.len()
            );
            resolved.extend(expanded);
        } else if let Some(event_name) = entry.event.as_deref() {
            let hook = resolve_direct_hook(entry.address, event_name, entry.bytes.len())
                .map_err(|err| anyhow!("mod {mod_id} patch[{idx}] hook error: {err}"))?;
            resolved.push(hook);
            modloader_debug!(
                "Resolved patch[{idx}] as direct trampoline hook for event '{}'",
                event_name
            );
        } else {
            resolved.push(ResolvedPatch::Bytes {
                address: entry.address,
                bytes: entry.bytes.clone(),
            });
            modloader_trace!(
                "Resolved patch[{idx}] as direct byte patch at 0x{:X}",
                entry.address
            );
        }
    }

    modloader_info!(
        "Finished resolving patches for mod '{}': {} resolved entries",
        mod_id,
        resolved.len()
    );
    Ok(resolved)
}

pub fn spans_for_patches(patches: &[ResolvedPatch]) -> Result<Vec<PatchSpan>> {
    modloader_trace!("Computing spans for {} resolved patches", patches.len());
    let mut spans = Vec::with_capacity(patches.len());
    for patch in patches {
        let len = patch.len();
        if len == 0 {
            return Err(anyhow!(
                "patch at 0x{:X} has empty byte payload",
                patch.address()
            ));
        }
        let end_exclusive = patch
            .address()
            .checked_add(len)
            .ok_or_else(|| anyhow!("patch range overflow at 0x{:X}", patch.address()))?;
        spans.push(PatchSpan {
            start: patch.address(),
            end_exclusive,
        });
        modloader_trace!(
            "Patch span: 0x{:X}-0x{:X} (len={})",
            patch.address(),
            end_exclusive,
            len
        );
    }
    Ok(spans)
}

pub fn ensure_no_overlap(
    current_mod: &str,
    spans: &[PatchSpan],
    existing: &[OwnedPatchSpan<'_>],
) -> Result<()> {
    for (idx, left) in spans.iter().enumerate() {
        for right in spans.iter().skip(idx + 1) {
            if spans_overlap(*left, *right) {
                modloader_warning!(
                    "Overlap detected within mod {}: {} overlaps {}",
                    current_mod,
                    left,
                    right
                );
                return Err(anyhow!(
                    "patch overlap in mod {current_mod}: range {} overlaps {}",
                    left,
                    right
                ));
            }
        }
    }

    for span in spans {
        for other in existing {
            if spans_overlap(*span, other.span) {
                modloader_warning!(
                    "Overlap detected between mod {} and {}: {} overlaps {}",
                    current_mod,
                    other.owner,
                    span,
                    other.span
                );
                return Err(anyhow!(
                    "patch overlap: mod {current_mod} range {} overlaps mod {} range {}",
                    span,
                    other.owner,
                    other.span
                ));
            }
        }
    }

    Ok(())
}

fn spans_overlap(left: PatchSpan, right: PatchSpan) -> bool {
    left.start < right.end_exclusive && right.start < left.end_exclusive
}

fn resolve_pattern_patch(
    address: usize,
    pattern: &str,
    patch_bytes: &[u8],
) -> Result<Vec<ResolvedPatch>> {
    modloader_trace!(
        "resolve_pattern_patch: address=0x{:X}, pattern='{}', bytes_len={}",
        address,
        pattern,
        patch_bytes.len()
    );
    let tokens: Vec<&str> = pattern.split_whitespace().collect();
    if tokens.is_empty() {
        return Err(anyhow!("pattern must not be empty"));
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
            return Err(anyhow!(
                "invalid wildcard token '{token}'; only full-byte wildcard '??' is allowed"
            ));
        }
        let expected = parse_hex_byte(token)
            .map_err(|err| anyhow!("invalid pattern byte '{token}': {err}"))?;
        let found = unsafe { ((address + idx) as *const u8).read() };
        if found != expected {
            return Err(anyhow!(
                "pattern mismatch at 0x{:X}: expected {:02X}, found {:02X}",
                address + idx,
                expected,
                found
            ));
        }
        idx += 1;
    }

    if wildcard_blocks.is_empty() {
        return Err(anyhow!("pattern has no wildcard blocks to patch"));
    }

    let wildcard_total: usize = wildcard_blocks.iter().map(|(_, len)| *len).sum();
    if patch_bytes.len() != wildcard_total {
        return Err(anyhow!(
            "pattern wildcard bytes mismatch: expected {} replacement bytes, got {}",
            wildcard_total,
            patch_bytes.len()
        ));
    }

    let mut consumed = 0usize;
    let mut resolved = Vec::new();
    for (start, len) in wildcard_blocks {
        let end = consumed + len;
        resolved.push(ResolvedPatch::Bytes {
            address: address + start,
            bytes: patch_bytes[consumed..end].to_vec(),
        });
        consumed = end;
    }
    Ok(resolved)
}

fn resolve_direct_hook(
    address: usize,
    event_name: &str,
    overwrite_len_hint: usize,
) -> Result<ResolvedPatch> {
    modloader_trace!(
        "resolve_direct_hook: address=0x{:X}, event='{}', overwrite_hint={}",
        address,
        event_name,
        overwrite_len_hint
    );
    let overwrite_len = overwrite_len_hint.max(5);
    let replaced_bytes: Vec<u8> = (0..overwrite_len)
        .map(|offset| unsafe { ((address + offset) as *const u8).read() })
        .collect();
    Ok(ResolvedPatch::TrampolineHook {
        address,
        overwrite_len,
        replaced_bytes,
        event_name: event_name.to_owned(),
    })
}

fn resolve_pattern_hook(address: usize, pattern: &str, event_name: &str) -> Result<ResolvedPatch> {
    modloader_trace!(
        "resolve_pattern_hook: address=0x{:X}, event='{}', pattern='{}'",
        address,
        event_name,
        pattern
    );
    let tokens: Vec<&str> = pattern.split_whitespace().collect();
    if tokens.is_empty() {
        return Err(anyhow!("pattern must not be empty"));
    }

    let mut first_wildcard_start = None;
    let mut wildcard_len = 0usize;

    let mut idx = 0usize;
    while idx < tokens.len() {
        let token = tokens[idx];
        if token == "??" {
            if first_wildcard_start.is_none() {
                first_wildcard_start = Some(idx);
            }
            wildcard_len += 1;
            idx += 1;
            continue;
        }
        if token.contains('?') {
            return Err(anyhow!(
                "invalid wildcard token '{token}'; only full-byte wildcard '??' is allowed"
            ));
        }
        let expected = parse_hex_byte(token)
            .map_err(|err| anyhow!("invalid pattern byte '{token}': {err}"))?;
        let found = unsafe { ((address + idx) as *const u8).read() };
        if found != expected {
            return Err(anyhow!(
                "pattern mismatch at 0x{:X}: expected {:02X}, found {:02X}",
                address + idx,
                expected,
                found
            ));
        }
        if first_wildcard_start.is_some() && wildcard_len > 0 {
            break;
        }
        idx += 1;
    }

    let start = first_wildcard_start.ok_or_else(|| {
        anyhow!("hook pattern must contain a wildcard block for trampoline insertion")
    })?;
    if wildcard_len < 5 {
        return Err(anyhow!(
            "hook pattern wildcard block too small: requires at least 5 bytes, got {}",
            wildcard_len
        ));
    }

    let hook_address = address + start;
    resolve_direct_hook(hook_address, event_name, wildcard_len)
}

fn parse_hex_byte(raw: &str) -> Result<u8> {
    if raw.len() != 2 {
        return Err(anyhow!("expected two hex digits"));
    }
    u8::from_str_radix(raw, 16).map_err(|err| anyhow!("invalid hex byte: {}", err))
}

fn deserialize_address<'de, D>(deserializer: D) -> std::result::Result<usize, D::Error>
where
    D: serde::Deserializer<'de>,
{
    #[derive(Deserialize)]
    #[serde(untagged)]
    enum AddressInput {
        Number(usize),
        String(String),
    }

    match AddressInput::deserialize(deserializer)? {
        AddressInput::Number(value) => Ok(value),
        AddressInput::String(value) => {
            let parsed = value
                .strip_prefix("0x")
                .or_else(|| value.strip_prefix("0X"))
                .map(|hex| usize::from_str_radix(hex, 16))
                .unwrap_or_else(|| value.parse());
            parsed.map_err(serde::de::Error::custom)
        }
    }
}

fn deserialize_hex_bytes<'de, D>(deserializer: D) -> std::result::Result<Vec<u8>, D::Error>
where
    D: serde::Deserializer<'de>,
{
    #[derive(Deserialize)]
    #[serde(untagged)]
    enum BytesInput {
        Array(Vec<u8>),
        String(String),
    }

    match BytesInput::deserialize(deserializer)? {
        BytesInput::Array(bytes) => Ok(bytes),
        BytesInput::String(raw) => raw
            .split_whitespace()
            .map(parse_hex_byte)
            .collect::<Result<Vec<u8>>>()
            .map_err(serde::de::Error::custom),
    }
}

/// Manages a set of patches; applies patches on creation and removes them on Drop.
pub struct Patches {
    handles: Vec<Handle>,
    hook_sites: Vec<usize>,
    suspend_process: bool,
}

impl Patches {
    pub fn new(entries: &[ResolvedPatch], suspend_process: bool) -> Self {
        modloader_debug!(
            "Creating Patches container with {} entries (suspend={})",
            entries.len(),
            suspend_process
        );
        let mut handles = Vec::new();
        let mut hook_sites = Vec::new();
        for (idx, entry) in entries.iter().enumerate() {
            match entry {
                ResolvedPatch::Bytes { address, bytes } => {
                    modloader_trace!(
                        "Applying patch[{}]: address=0x{:X}, bytes={:02X?}",
                        idx,
                        address,
                        bytes
                    );
                    let handle = make_patch_with_suspend(*address, bytes, suspend_process);
                    modloader_debug!("Patch[{}] applied with handle {}", idx, handle);
                    handles.push(handle);
                }
                ResolvedPatch::TrampolineHook {
                    address,
                    overwrite_len,
                    replaced_bytes,
                    event_name,
                } => {
                    modloader_trace!(
                        "Applying hook patch[{}]: address=0x{:X}, overwrite_len={}, event={}",
                        idx,
                        address,
                        overwrite_len,
                        event_name
                    );
                    if let Err(err) = hook::register_trampoline_hook(
                        *address,
                        event_name.clone(),
                        replaced_bytes.clone(),
                        suspend_process,
                    ) {
                        modloader_debug!("Hook registration failed for patch[{}]: {}", idx, err);
                    } else {
                        hook_sites.push(*address);
                    }
                }
            }
        }
        modloader_debug!("All patch entries applied ({} handles)", handles.len());
        Patches {
            handles,
            hook_sites,
            suspend_process,
        }
    }
}

impl Drop for Patches {
    fn drop(&mut self) {
        // Remove all patches on drop
        modloader_debug!(
            "Dropping Patches container ({} handles)",
            self.handles.len()
        );
        for &handle in &self.handles {
            modloader_trace!("Removing patch handle {}", handle);
            let _ = remove_patch_with_suspend(handle, self.suspend_process);
        }
        for &site in &self.hook_sites {
            hook::unregister_trampoline_hook(site);
        }
        modloader_debug!("Finished removing patch handles");
    }
}
