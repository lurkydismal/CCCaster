use semver::{Version, VersionReq};
use serde::Deserialize;

/// Intermediate structure for deserializing info.json
#[derive(Deserialize)]
pub struct RawDependency {
    /// Dependency mod id.
    pub id: String,
    /// Raw semver requirement expression (e.g. ">=1.2, <2.0").
    pub version_req: String,
    /// Optional dependencies do not block load order resolution.
    #[serde(default)]
    pub optional: bool,
}

/// Raw JSON payload from `addons/<mod>/info.json`.
#[derive(Deserialize)]
pub struct RawModInfo {
    /// Semantic version string parsed into [`Version`].
    pub version: String,
    #[serde(default)]
    pub dependencies: Vec<RawDependency>,
    #[serde(default)]
    pub api_version: u8,
    #[serde(default)]
    pub events: Vec<String>,
    #[serde(default)]
    pub assets: RawAssetsConfig,
}

#[derive(Deserialize, Default, Clone)]
pub struct RawAssetsConfig {
    #[serde(default)]
    pub ignore: Vec<String>,
    #[serde(default)]
    pub convert: std::collections::HashMap<String, std::collections::HashMap<String, String>>,
}

/// Resolved dependency with semver version requirement.
#[derive(Clone)]
pub struct Dependency {
    /// Dependency mod id.
    pub id: String,
    /// Parsed semver requirement.
    pub version_req: VersionReq,
    /// Whether dependency is best-effort only.
    pub optional: bool,
}

/// Metadata for a mod, extracted from info.json
#[derive(Clone)]
pub struct ModMeta {
    /// Unique mod id.
    pub id: String,
    /// Parsed semantic version.
    pub version: Version,
    /// Dependency requirements for load order computation.
    pub dependencies: Vec<Dependency>,
    /// Host API compatibility level.
    pub api_version: u8,
    /// Declared event hooks for future dispatch layers.
    pub events: Vec<String>,
    /// Asset loading options.
    pub assets: RawAssetsConfig,
}
