use semver::{Version, VersionReq};
use serde::Deserialize;

/// Intermediate structure for deserializing info.json
#[derive(Deserialize)]
pub struct RawDependency {
    pub id: String,
    pub version_req: String,
    #[serde(default)]
    pub optional: bool,
}

#[derive(Deserialize)]
pub struct RawModInfo {
    pub id: String,
    pub version: String,
    #[serde(default)]
    pub dependencies: Vec<RawDependency>,
    #[serde(default)]
    pub api_version: u8,
    #[serde(default)]
    pub events: Vec<String>,
}

/// Resolved dependency with semver version requirement.
#[derive(Clone)]
pub struct Dependency {
    pub id: String,
    pub version_req: VersionReq,
    pub optional: bool,
}

/// Metadata for a mod, extracted from info.json
#[derive(Clone)]
pub struct ModMeta {
    pub id: String,
    pub version: Version,
    pub dependencies: Vec<Dependency>,
    pub api_version: u8,
    pub events: Vec<String>,
}
