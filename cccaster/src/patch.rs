use crate::api::{Handle, make_patch, remove_patch};
use crate::{modloader_debug, modloader_trace};
use serde::Deserialize;
use std::fmt;

use anyhow::{Result, anyhow};

/// Represents a patch entry from patch.json
#[derive(Deserialize)]
pub struct PatchEntry {
    #[serde(deserialize_with = "deserialize_address")]
    pub address: usize,
    #[serde(deserialize_with = "deserialize_hex_bytes")]
    pub bytes: Vec<u8>,
    #[serde(default)]
    pub pattern: Option<String>,
}

#[derive(Clone)]
pub struct ResolvedPatch {
    pub address: usize,
    pub bytes: Vec<u8>,
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
    let mut resolved = Vec::new();

    for (idx, entry) in entries.iter().enumerate() {
        if let Some(pattern) = entry.pattern.as_deref() {
            let expanded = resolve_pattern_patch(entry.address, pattern, &entry.bytes)
                .map_err(|err| anyhow!("mod {mod_id} patch[{idx}] pattern error: {err}"))?;
            resolved.extend(expanded);
        } else {
            resolved.push(ResolvedPatch {
                address: entry.address,
                bytes: entry.bytes.clone(),
            });
        }
    }

    Ok(resolved)
}

pub fn spans_for_patches(patches: &[ResolvedPatch]) -> Result<Vec<PatchSpan>> {
    let mut spans = Vec::with_capacity(patches.len());
    for patch in patches {
        if patch.bytes.is_empty() {
            return Err(anyhow!(
                "patch at 0x{:X} has empty byte payload",
                patch.address
            ));
        }
        let end_exclusive = patch
            .address
            .checked_add(patch.bytes.len())
            .ok_or_else(|| anyhow!("patch range overflow at 0x{:X}", patch.address))?;
        spans.push(PatchSpan {
            start: patch.address,
            end_exclusive,
        });
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
        resolved.push(ResolvedPatch {
            address: address + start,
            bytes: patch_bytes[consumed..end].to_vec(),
        });
        consumed = end;
    }
    Ok(resolved)
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
}

impl Patches {
    pub fn new(entries: &[ResolvedPatch]) -> Self {
        modloader_debug!("Creating Patches container with {} entries", entries.len());
        let mut handles = Vec::new();
        for (idx, entry) in entries.iter().enumerate() {
            modloader_trace!(
                "Applying patch[{}]: address=0x{:X}, bytes={:02X?}",
                idx,
                entry.address,
                entry.bytes
            );
            // Apply patch by calling into the provided API
            let handle = make_patch(entry.address, &entry.bytes);
            modloader_debug!("Patch[{}] applied with handle {}", idx, handle);
            handles.push(handle);
        }
        modloader_debug!("All patch entries applied ({} handles)", handles.len());
        Patches { handles }
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
            let _ = remove_patch(handle);
        }
        modloader_debug!("Finished removing patch handles");
    }
}
