use anyhow::{Context, Result};
use fuser::{
    Errno, FileAttr, FileHandle, FileType, Filesystem, FopenFlags, Generation, INodeNo, LockOwner,
    MountOption, OpenFlags, ReplyAttr, ReplyData, ReplyDirectory, ReplyEntry, ReplyOpen, Request,
};
use globset::{Glob, GlobSet, GlobSetBuilder};
use once_cell::sync::OnceCell;
use std::collections::{BTreeSet, HashMap};
use std::ffi::OsStr;
use std::fs;
use std::os::unix::fs::MetadataExt;
use std::path::{Path, PathBuf};
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::{Duration, SystemTime};

const TTL: Duration = Duration::from_secs(1);

#[derive(Clone)]
struct ModAssetSource {
    root: PathBuf,
    ignore: GlobSet,
}

pub struct OverlayRegistry {
    launcher_root: PathBuf,
    passthrough_root: PathBuf,
    whitelist: GlobSet,
    global_entries: HashMap<String, Vec<u8>>,
    assets: Vec<ModAssetSource>,
}

impl OverlayRegistry {
    fn new(launcher_root: PathBuf, whitelist: GlobSet) -> Self {
        Self {
            passthrough_root: launcher_root.clone(),
            launcher_root,
            whitelist,
            global_entries: HashMap::new(),
            assets: Vec::new(),
        }
    }

    fn add_mod_assets(&mut self, mod_path: &Path, ignore_patterns: &[String]) -> Result<()> {
        let assets_root = mod_path.join("assets");
        if !assets_root.exists() {
            return Ok(());
        }
        let canonical_assets = assets_root
            .canonicalize()
            .with_context(|| format!("failed to canonicalize {}", assets_root.display()))?;
        if !canonical_assets.starts_with(&self.launcher_root) {
            anyhow::bail!(
                "mod assets path {} escapes launcher root {}",
                canonical_assets.display(),
                self.launcher_root.display()
            );
        }

        let ignore = build_globset(ignore_patterns)?;
        self.assets.push(ModAssetSource {
            root: canonical_assets,
            ignore,
        });
        Ok(())
    }

    fn write_global(&mut self, path: String, content: Vec<u8>, append: bool) {
        if append {
            self.global_entries.entry(path).or_default().extend(content);
        } else {
            self.global_entries.insert(path, content);
        }
    }

    fn read_global(&self, path: &str) -> Option<String> {
        self.read_overlay(path)
            .map(|bytes| String::from_utf8_lossy(&bytes).into_owned())
    }

    fn read_overlay(&self, rel_path: &str) -> Option<Vec<u8>> {
        if let Some(bytes) = self.global_entries.get(rel_path) {
            return Some(bytes.clone());
        }

        for source in self.assets.iter().rev() {
            if source.ignore.is_match(rel_path) {
                continue;
            }
            let candidate = source.root.join(rel_path);
            if let Ok(canonical) = candidate.canonicalize() {
                if !canonical.starts_with(&source.root) {
                    continue;
                }
                if let Ok(data) = fs::read(&canonical) {
                    return Some(data);
                }
            }
        }

        None
    }

    fn passthrough_path(&self, rel: &str) -> PathBuf {
        self.passthrough_root.join(rel)
    }

    fn is_passthrough_excluded(&self, rel: &str) -> bool {
        self.whitelist.is_match(rel)
    }
}

static REGISTRY: OnceCell<Arc<Mutex<OverlayRegistry>>> = OnceCell::new();

pub fn init_overlay_registry(launcher_root: &Path) -> Result<()> {
    let whitelist_path = launcher_root.join("whitelist.json");
    let patterns: Vec<String> = if whitelist_path.exists() {
        let bytes = fs::read(&whitelist_path)
            .with_context(|| format!("failed reading {}", whitelist_path.display()))?;
        serde_json::from_slice(&bytes)
            .with_context(|| format!("failed parsing {}", whitelist_path.display()))?
    } else {
        Vec::new()
    };

    let whitelist = build_globset(&patterns)?;
    let registry = OverlayRegistry::new(launcher_root.canonicalize()?, whitelist);
    let _ = REGISTRY.set(Arc::new(Mutex::new(registry)));
    Ok(())
}

pub fn register_mod_assets(mod_path: &Path, ignore: &[String]) -> Result<()> {
    if let Some(state) = REGISTRY.get() {
        state
            .lock()
            .map_err(|_| anyhow::anyhow!("overlay registry mutex poisoned"))?
            .add_mod_assets(mod_path, ignore)?;
    }
    Ok(())
}

pub fn read_global(path: &str) -> Option<String> {
    REGISTRY
        .get()
        .and_then(|state| state.lock().ok())
        .and_then(|state| state.read_global(path))
}

pub fn write_global(path: String, content: Vec<u8>, append: bool) -> Result<()> {
    if let Some(state) = REGISTRY.get() {
        state
            .lock()
            .map_err(|_| anyhow::anyhow!("overlay registry mutex poisoned"))?
            .write_global(path, content, append);
    }
    Ok(())
}

#[cfg(target_os = "linux")]
pub fn mount_process_local_overlay() -> Result<()> {
    let state = match REGISTRY.get() {
        Some(value) => value.clone(),
        None => return Ok(()),
    };

    unsafe {
        if libc::unshare(libc::CLONE_NEWUSER) != 0 {
            anyhow::bail!(
                "unshare(CLONE_NEWUSER) failed: {}",
                std::io::Error::last_os_error()
            );
        }

        // deny setgroups if needed
        // write("/proc/self/setgroups", "deny");

        // write("/proc/self/uid_map", "0 <your_uid> 1");
        // write("/proc/self/gid_map", "0 <your_gid> 1");
        // write uid_map / gid_map here

        if libc::unshare(libc::CLONE_NEWNS) != 0 {
            anyhow::bail!(
                "unshare(CLONE_NEWNS) failed: {}",
                std::io::Error::last_os_error()
            );
        }
        if libc::mount(
            std::ptr::null(),
            c"/".as_ptr(),
            std::ptr::null(),
            (libc::MS_REC | libc::MS_PRIVATE) as libc::c_ulong,
            std::ptr::null(),
        ) != 0
        {
            anyhow::bail!(
                "mount(/, MS_REC|MS_PRIVATE) failed: {}",
                std::io::Error::last_os_error()
            );
        }
    }

    let mountpoint = std::env::temp_dir().join(format!("cccaster-overlay-{}", std::process::id()));
    fs::create_dir_all(&mountpoint)?;

    let fs_impl = OverlayFs::new(state.clone());
    let options = vec![
        MountOption::RO,
        MountOption::FSName("cccaster-overlay".to_string()),
        MountOption::AutoUnmount,
    ];
    let mut config = fuser::Config::default();
    config.mount_options = options;

    let session = fuser::spawn_mount2(fs_impl, &mountpoint, &config)?;
    thread::spawn(move || {
        let _session = session;
        loop {
            thread::park();
        }
    });

    std::env::set_current_dir(&mountpoint)?;
    Ok(())
}

#[cfg(not(target_os = "linux"))]
pub fn mount_process_local_overlay() -> Result<()> {
    Ok(())
}

fn build_globset(patterns: &[String]) -> Result<GlobSet> {
    let mut builder = GlobSetBuilder::new();
    for pattern in patterns {
        builder.add(Glob::new(pattern)?);
    }
    Ok(builder.build()?)
}

fn normalize_rel(path: &Path) -> Option<String> {
    let mut parts = Vec::new();
    for component in path.components() {
        match component {
            std::path::Component::Normal(value) => parts.push(value.to_string_lossy().to_string()),
            std::path::Component::CurDir => {}
            _ => return None,
        }
    }
    Some(parts.join("/"))
}

fn real_attr(path: &Path, ino: INodeNo) -> Option<FileAttr> {
    let meta = fs::metadata(path).ok()?;
    Some(FileAttr {
        ino,
        size: meta.len(),
        blocks: meta.blocks(),
        atime: SystemTime::UNIX_EPOCH + Duration::from_secs(meta.atime() as u64),
        mtime: SystemTime::UNIX_EPOCH + Duration::from_secs(meta.mtime() as u64),
        ctime: SystemTime::UNIX_EPOCH + Duration::from_secs(meta.ctime() as u64),
        crtime: SystemTime::UNIX_EPOCH,
        kind: if meta.is_dir() {
            FileType::Directory
        } else {
            FileType::RegularFile
        },
        perm: if meta.is_dir() { 0o755 } else { 0o644 },
        nlink: 1,
        uid: meta.uid(),
        gid: meta.gid(),
        rdev: 0,
        flags: 0,
        blksize: 4096,
    })
}

struct OverlayFs {
    state: Arc<Mutex<OverlayRegistry>>,
    inodes: Mutex<HashMap<INodeNo, String>>,
    paths: Mutex<HashMap<String, INodeNo>>,
    next_ino: Mutex<INodeNo>,
}

impl OverlayFs {
    fn new(state: Arc<Mutex<OverlayRegistry>>) -> Self {
        let mut inodes = HashMap::new();
        let mut paths = HashMap::new();
        inodes.insert(INodeNo(1), String::new());
        paths.insert(String::new(), INodeNo(1));
        Self {
            state,
            inodes: Mutex::new(inodes),
            paths: Mutex::new(paths),
            next_ino: Mutex::new(INodeNo(2)),
        }
    }

    fn inode_for(&self, path: &str) -> INodeNo {
        if let Ok(paths) = self.paths.lock()
            && let Some(value) = paths.get(path)
        {
            return *value;
        }

        let mut next = self.next_ino.lock().expect("inode mutex");
        let ino = *next;

        next.0 = next.0.checked_add(1).expect("inode overflow");

        self.paths
            .lock()
            .expect("paths mutex")
            .insert(path.to_string(), ino);
        self.inodes
            .lock()
            .expect("inodes mutex")
            .insert(ino, path.to_string());
        ino
    }

    fn rel_for_inode(&self, ino: INodeNo) -> Option<String> {
        self.inodes.lock().ok().and_then(|m| m.get(&ino).cloned())
    }
}

impl Filesystem for OverlayFs {
    fn lookup(&self, _req: &Request, parent: INodeNo, name: &OsStr, reply: ReplyEntry) {
        let parent_rel = match self.rel_for_inode(parent) {
            Some(path) => path,
            None => return reply.error(Errno::ENOENT),
        };
        let mut child = PathBuf::from(&parent_rel);
        child.push(name);
        let Some(rel) = normalize_rel(&child) else {
            return reply.error(Errno::ENOENT);
        };

        let ino = self.inode_for(&rel);
        let state = self.state.lock().expect("state mutex");
        if let Some(data) = state.read_overlay(&rel) {
            let attr = FileAttr {
                ino,
                size: data.len() as u64,
                blocks: 1,
                atime: SystemTime::UNIX_EPOCH,
                mtime: SystemTime::UNIX_EPOCH,
                ctime: SystemTime::UNIX_EPOCH,
                crtime: SystemTime::UNIX_EPOCH,
                kind: FileType::RegularFile,
                perm: 0o644,
                nlink: 1,
                uid: 0,
                gid: 0,
                rdev: 0,
                flags: 0,
                blksize: 4096,
            };
            return reply.entry(&TTL, &attr, Generation(0));
        }

        let real = state.passthrough_path(&rel);
        if let Some(attr) = real_attr(&real, ino) {
            return reply.entry(&TTL, &attr, Generation(0));
        }

        reply.error(Errno::ENOENT)
    }

    fn getattr(&self, _req: &Request, ino: INodeNo, _fh: Option<FileHandle>, reply: ReplyAttr) {
        let Some(rel) = self.rel_for_inode(ino) else {
            return reply.error(Errno::ENOENT);
        };

        if rel.is_empty() {
            let attr = FileAttr {
                ino: INodeNo(1),
                size: 0,
                blocks: 0,
                atime: SystemTime::UNIX_EPOCH,
                mtime: SystemTime::UNIX_EPOCH,
                ctime: SystemTime::UNIX_EPOCH,
                crtime: SystemTime::UNIX_EPOCH,
                kind: FileType::Directory,
                perm: 0o755,
                nlink: 2,
                uid: 0,
                gid: 0,
                rdev: 0,
                flags: 0,
                blksize: 4096,
            };
            return reply.attr(&TTL, &attr);
        }

        let state = self.state.lock().expect("state mutex");
        if let Some(data) = state.read_overlay(&rel) {
            let attr = FileAttr {
                ino,
                size: data.len() as u64,
                blocks: 1,
                atime: SystemTime::UNIX_EPOCH,
                mtime: SystemTime::UNIX_EPOCH,
                ctime: SystemTime::UNIX_EPOCH,
                crtime: SystemTime::UNIX_EPOCH,
                kind: FileType::RegularFile,
                perm: 0o644,
                nlink: 1,
                uid: 0,
                gid: 0,
                rdev: 0,
                flags: 0,
                blksize: 4096,
            };
            return reply.attr(&TTL, &attr);
        }

        let real = state.passthrough_path(&rel);
        if let Some(attr) = real_attr(&real, ino) {
            return reply.attr(&TTL, &attr);
        }

        reply.error(Errno::ENOENT)
    }

    fn readdir(
        &self,
        _req: &Request,
        ino: INodeNo,
        _fh: FileHandle,
        offset: u64,
        mut reply: ReplyDirectory,
    ) {
        let Some(rel) = self.rel_for_inode(ino) else {
            return reply.error(Errno::ENOENT);
        };
        let state = self.state.lock().expect("state mutex");
        let dir = state.passthrough_path(&rel);
        if !dir.is_dir() {
            return reply.error(Errno::ENOENT);
        }

        let mut entries: BTreeSet<String> = BTreeSet::new();
        if let Ok(read_dir) = fs::read_dir(&dir) {
            for entry in read_dir.flatten() {
                if let Some(name) = entry.file_name().to_str() {
                    let full_rel = if rel.is_empty() {
                        name.to_string()
                    } else {
                        format!("{rel}/{name}")
                    };
                    if state.is_passthrough_excluded(&full_rel) {
                        continue;
                    }
                    entries.insert(name.to_string());
                }
            }
        }

        for key in state.global_entries.keys() {
            let mut parts = key.split('/');
            if let Some(first) = parts.next() {
                if rel.is_empty() {
                    entries.insert(first.to_string());
                } else if key.starts_with(&format!("{rel}/"))
                    && let Some(remain) = key.strip_prefix(&format!("{rel}/"))
                    && let Some(name) = remain.split('/').next()
                {
                    entries.insert(name.to_string());
                }
            }
        }

        let mut offset_idx = 0u64;
        if offset <= offset_idx {
            let _ = reply.add(ino, offset_idx + 1, FileType::Directory, ".");
        }
        offset_idx += 1;
        if offset <= offset_idx {
            let _ = reply.add(INodeNo(1), offset_idx + 1, FileType::Directory, "..");
        }

        for name in entries.into_iter().skip(offset as usize) {
            offset_idx += 1;
            let child_rel = if rel.is_empty() {
                name.clone()
            } else {
                format!("{rel}/{name}")
            };
            let child_ino = self.inode_for(&child_rel);
            let file_type = if state.passthrough_path(&child_rel).is_dir() {
                FileType::Directory
            } else {
                FileType::RegularFile
            };
            if reply.add(child_ino, offset_idx + 1, file_type, name) {
                break;
            }
        }

        reply.ok();
    }

    fn open(&self, _req: &Request, ino: INodeNo, _flags: OpenFlags, reply: ReplyOpen) {
        let Some(rel) = self.rel_for_inode(ino) else {
            return reply.error(Errno::ENOENT);
        };
        let state = self.state.lock().expect("state mutex");
        if state.read_overlay(&rel).is_some() || state.passthrough_path(&rel).is_file() {
            // WARN: Enable cache
            return reply.opened(FileHandle(0), FopenFlags::FOPEN_DIRECT_IO);
        }
        if state.passthrough_path(&rel).is_dir() {
            return reply.error(Errno::EISDIR);
        }
        reply.error(Errno::ENOENT)
    }

    fn read(
        &self,
        _req: &Request,
        ino: INodeNo,
        _fh: FileHandle,
        offset: u64,
        size: u32,
        _flags: OpenFlags,
        _lock_owner: Option<LockOwner>,
        reply: ReplyData,
    ) {
        let Some(rel) = self.rel_for_inode(ino) else {
            return reply.error(Errno::ENOENT);
        };

        let state = self.state.lock().expect("state mutex");
        let data = state
            .read_overlay(&rel)
            .or_else(|| fs::read(state.passthrough_path(&rel)).ok());

        let Some(data) = data else {
            return reply.error(Errno::ENOENT);
        };
        let start = offset as usize;
        let end = (start + size as usize).min(data.len());
        if start >= data.len() {
            return reply.data(&[]);
        }
        reply.data(&data[start..end]);
    }
}
