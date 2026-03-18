#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "addon_types.hpp"
// #include "watch.hpp"

namespace mod {

class AddonLoaderT final {
public:
    explicit AddonLoaderT( std::filesystem::path _addonsRoot );

    auto scan() -> bool;
    auto loadAll() -> bool;
    auto unloadAll() -> void;
    // auto tick( bool _blocking ) -> void;
    auto requestReload() -> void;

    auto dispatchEvent( std::string_view _event ) -> void;

    [[nodiscard]] auto addons() const
        -> const std::unordered_map< std::string, addonRecordT >& {
        return _mAddons;
    }

private:
    static auto _parseHexU64( std::string_view _text )
        -> std::optional< std::uint64_t >;
    static auto _validateBytesString( std::string_view _text ) -> bool;
    static auto _validatePatternString( std::string_view _text ) -> bool;

    static auto _safeUnderRoot( const std::filesystem::path& _root,
                                const std::filesystem::path& _path ) -> bool;

    static auto _validateSafeName( std::string_view _name ) -> bool;
    static auto _readTextFile( const std::filesystem::path& _path,
                               std::string& _out ) -> bool;
    static auto _parseDependencyString( std::string_view _text, bool _optional )
        -> std::optional< dependencyT >;

    auto _loadManifest( addonRecordT& _addon ) -> bool;
    auto _loadPatchManifest( addonRecordT& _addon ) -> bool;
    auto _buildGraph() -> bool;
    auto _topoSort() -> bool;
    auto _canLoadAddon( const addonRecordT& _addon ) const -> bool;
    auto _createRuntime( const addonRecordT& _addon )
        -> std::unique_ptr< addonRecordT::runtimeT >;

    auto _computeFingerprint( const std::filesystem::path& _root ) const
        -> std::uint64_t;
    auto _unloadDependentsFirst( const std::string& _id ) -> void;
    auto _reloadAll() -> void;

    static auto _parsePatchBytes( std::string_view _text )
        -> std::optional< std::vector< std::byte > >;

    auto _applyPatches( addonRecordT& _addon ) -> bool;

private:
    std::filesystem::path _mAddonsRoot{};
    std::unordered_map< std::string, addonRecordT > _mAddons{};
    std::vector< std::string > _mLoadOrder{};
    std::unordered_map< std::string, std::vector< std::string > >
        _mReverseDeps{};
    bool _mReloadRequested{ false };
    // std::unique_ptr< watch::watch_t > m_watch{};
    std::unordered_set< std::uintptr_t > _mUsedPatchAddresses{};
    std::unordered_set< std::string > _mUsedPatchPatterns{};
};

} // namespace mod
