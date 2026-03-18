#include "addon_loader.hpp"

#include <glaze/glaze.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <iostream>
#include <queue>
#include <ranges>

#include "addon_types.hpp"
#include "lua_runtime.hpp"
#include "stdhash.hpp"
#include "store.hpp"
#include "version.hpp"

namespace mod {

static constexpr std::uint32_t g_engineApiVersion = 1;

auto AddonLoaderT::_parsePatchBytes( std::string_view _text )
    -> std::optional< std::vector< std::byte > > {
    std::vector< std::byte > l_out{};

    while ( !_text.empty() &&
            std::isspace( static_cast< unsigned char >( _text.front() ) ) ) {
        _text.remove_prefix( 1 );
    }

    while ( !_text.empty() &&
            std::isspace( static_cast< unsigned char >( _text.back() ) ) ) {
        _text.remove_suffix( 1 );
    }

    if ( _text.empty() ) {
        return ( std::nullopt );
    }

    std::size_t l_pos = 0;

    while ( l_pos < _text.size() ) {
        while (
            l_pos < _text.size() &&

            std::isspace( static_cast< unsigned char >( _text[ l_pos ] ) ) ) {
            ++l_pos;
        }

        if ( l_pos >= _text.size() ) {
            break;
        }

        const auto l_start = l_pos;

        while (
            l_pos < _text.size() &&
            !std::isspace( static_cast< unsigned char >( _text[ l_pos ] ) ) ) {
            ++l_pos;
        }

        const auto l_token = _text.substr( l_start, l_pos - l_start );

        if ( l_token.size() != 2 ) {
            return ( std::nullopt );
        }

        auto l_isHex = []( char _c ) -> bool {
            return ( std::isxdigit( static_cast< unsigned char >( _c ) ) != 0 );
        };

        if ( !l_isHex( l_token[ 0 ] ) || !l_isHex( l_token[ 1 ] ) ) {
            return ( std::nullopt );
        }

        const auto l_value = static_cast< unsigned int >(
            std::strtoul( std::string( l_token ).c_str(), nullptr, 16 ) );

        l_out.push_back( static_cast< std::byte >( l_value ) );
    }

    if ( l_out.empty() ) {
        return ( std::nullopt );
    }

    return ( l_out );
}

auto AddonLoaderT::_applyPatches( addonRecordT& _addon ) -> bool {
    for ( const auto& l_patch : _addon.patches ) {
        const bool l_hasPattern = l_patch.pattern.has_value();

        auto l_bytes = _parsePatchBytes( l_patch.bytes );

        if ( !l_bytes ) {
            _addon.error = "Invalid patch bytes";

            return ( false );
        }

        auto l_address = _parseHexU64( l_patch.address );

        if ( !l_address ) {
            _addon.error = "Invalid patch address";

            return ( false );
        }

        if ( !_mUsedPatchAddresses
                  .insert( static_cast< std::uintptr_t >( l_address.value() ) )
                  .second ) {
            _addon.error = "Patch address conflict";

            return ( false );
        }

        if ( l_hasPattern ) {
            _addon.applied_patches.push_back( store::g_api.makePatch(
                static_cast< std::uintptr_t >( l_address.value() ),
                l_patch.pattern.value(),
                std::span< const std::byte >( l_bytes->data(),
                                              l_bytes->size() ) ) );

        } else {
            _addon.applied_patches.push_back( store::g_api.makePatch(
                static_cast< std::uintptr_t >( l_address.value() ),
                std::span< const std::byte >( l_bytes->data(),
                                              l_bytes->size() ) ) );
        }
    }

    return ( true );
}

AddonLoaderT::AddonLoaderT( std::filesystem::path _addonsRoot )
    : _mAddonsRoot( std::move( _addonsRoot ) ) {}

auto AddonLoaderT::_parseHexU64( std::string_view _text )
    -> std::optional< std::uint64_t > {
    while ( !_text.empty() &&
            std::isspace( static_cast< unsigned char >( _text.front() ) ) ) {
        _text.remove_prefix( 1 );
    }

    while ( !_text.empty() &&
            std::isspace( static_cast< unsigned char >( _text.back() ) ) ) {
        _text.remove_suffix( 1 );
    }

    if ( _text.starts_with( "0x" ) || _text.starts_with( "0X" ) ) {
        _text.remove_prefix( 2 );
    }

    if ( _text.empty() ) {
        return ( std::nullopt );
    }

    std::uint64_t l_value{};

    auto l_res = std::from_chars( _text.begin(), _text.end(), l_value, 16 );

    if ( l_res.ec != std::errc{} || l_res.ptr != _text.end() ) {
        return ( std::nullopt );
    }

    return ( l_value );
}

auto AddonLoaderT::_validateBytesString( std::string_view _text ) -> bool {
    while ( !_text.empty() &&
            std::isspace( static_cast< unsigned char >( _text.front() ) ) ) {
        _text.remove_prefix( 1 );
    }

    while ( !_text.empty() &&
            std::isspace( static_cast< unsigned char >( _text.back() ) ) ) {
        _text.remove_suffix( 1 );
    }

    if ( _text.empty() ) {
        return ( false );
    }

    std::size_t l_count = 0;
    std::size_t l_pos = 0;

    while ( l_pos < _text.size() ) {
        while (
            l_pos < _text.size() &&
            std::isspace( static_cast< unsigned char >( _text[ l_pos ] ) ) ) {
            ++l_pos;
        }

        if ( l_pos >= _text.size() ) {
            break;
        }

        const auto l_start = l_pos;

        while (
            l_pos < _text.size() &&
            !std::isspace( static_cast< unsigned char >( _text[ l_pos ] ) ) ) {
            ++l_pos;
        }

        const auto l_token = _text.substr( l_start, l_pos - l_start );

        if ( l_token.size() != 2 ) {
            return ( false );
        }

        auto l_isHex = []( char _c ) -> bool {
            return ( std::isxdigit( static_cast< unsigned char >( _c ) ) != 0 );
        };

        if ( !l_isHex( l_token[ 0 ] ) || !l_isHex( l_token[ 1 ] ) ) {
            return ( false );
        }

        ++l_count;
    }

    return ( l_count != 0 );
}

auto AddonLoaderT::_validatePatternString( std::string_view _text ) -> bool {
    while ( !_text.empty() &&
            std::isspace( static_cast< unsigned char >( _text.front() ) ) ) {
        _text.remove_prefix( 1 );
    }

    while ( !_text.empty() &&
            std::isspace( static_cast< unsigned char >( _text.back() ) ) ) {
        _text.remove_suffix( 1 );
    }

    return ( !_text.empty() );
}

auto AddonLoaderT::_safeUnderRoot( const std::filesystem::path& _root,
                                   const std::filesystem::path& _path )
    -> bool {
    std::error_code l_ec{};

    const auto l_root = std::filesystem::weakly_canonical( _root, l_ec );

    if ( l_ec ) {
        return ( false );
    }

    const auto l_path = std::filesystem::weakly_canonical( _path, l_ec );

    if ( l_ec ) {
        return ( false );
    }

    auto l_rootIt = l_root.begin();
    auto l_pathIt = l_path.begin();

    for ( ; l_rootIt != l_root.end() && l_pathIt != l_path.end();
          ++l_rootIt, ++l_pathIt ) {
        if ( *l_rootIt != *l_pathIt ) {
            return ( false );
        }
    }

    return ( l_rootIt == l_root.end() );
}

auto AddonLoaderT::_validateSafeName( std::string_view _name ) -> bool {
    if ( _name.empty() ) {
        return ( false );
    }

    for ( const char l_c : _name ) {
        const auto l_uc = static_cast< unsigned char >( l_c );

        if ( std::isalnum( l_uc ) || l_c == '_' || l_c == '-' || l_c == '.' ) {
            continue;
        }
        return ( false );
    }

    return ( _name != "." && _name != ".." );
}

auto AddonLoaderT::_readTextFile( const std::filesystem::path& _path,
                                  std::string& _out ) -> bool {
    std::ifstream l_ifs( _path, std::ios::binary );

    if ( !l_ifs ) {
        return ( false );
    }

    l_ifs.seekg( 0, std::ios::end );

    const auto l_size = static_cast< std::size_t >( l_ifs.tellg() );

    l_ifs.seekg( 0, std::ios::beg );

    _out.resize( l_size );

    if ( l_size != 0 ) {
        l_ifs.read( _out.data(), static_cast< std::streamsize >( l_size ) );
    }

    return ( static_cast< bool >( l_ifs ) || l_ifs.eof() );
}

auto AddonLoaderT::_parseDependencyString( std::string_view _text,
                                           bool _optional )
    -> std::optional< dependencyT > {
    while ( !_text.empty() &&
            std::isspace( static_cast< unsigned char >( _text.front() ) ) ) {
        _text.remove_prefix( 1 );
    }

    while ( !_text.empty() &&
            std::isspace( static_cast< unsigned char >( _text.back() ) ) ) {
        _text.remove_suffix( 1 );
    }

    if ( _text.empty() ) {
        return ( std::nullopt );
    }

    const auto l_space = _text.find_first_of( " \t" );

    dependencyT l_out{};

    if ( l_space == std::string_view::npos ) {
        l_out.name = std::string( _text );
        l_out.range.kind = versionRangeT::kindT::any;
        l_out.optional = _optional;

        return ( l_out );
    }

    l_out.name = std::string( _text.substr( 0, l_space ) );
    std::string_view l_rest = _text.substr( l_space + 1 );

    while ( !l_rest.empty() &&
            std::isspace( static_cast< unsigned char >( l_rest.front() ) ) ) {
        l_rest.remove_prefix( 1 );
    }

    if ( l_rest.empty() ) {
        l_out.range.kind = versionRangeT::kindT::any;
        l_out.optional = _optional;

        return ( l_out );
    }

    auto l_range = parseVersionRange( l_rest );

    if ( !l_range ) {
        return ( std::nullopt );
    }

    l_out.range = *l_range;
    l_out.optional = _optional;

    return ( l_out );
}

auto AddonLoaderT::_loadManifest( addonRecordT& _addon ) -> bool {
    const auto l_infoPath = _addon.root / "info.json";

    std::string l_text{};

    if ( !_readTextFile( l_infoPath, l_text ) ) {
        _addon.error = "Missing info.json";

        return ( false );
    }

    auto l_ec = glz::read_json( _addon.manifest, l_text );

    if ( l_ec ) {
        _addon.error = "Invalid info.json";

        return ( false );
    }

    if ( _addon.manifest.author.empty() || _addon.manifest.version.empty() ||
         _addon.manifest.runtime.empty() || _addon.manifest.entry.empty() ) {
        _addon.error = "info.json missing required fields";

        return ( false );
    }

    if ( !_validateSafeName( _addon.manifest.runtime ) ) {
        _addon.error = "Invalid runtime";

        return ( false );
    }

    if ( !_safeUnderRoot( _addon.root, _addon.root / _addon.manifest.entry ) ) {
        _addon.error = "Invalid entry path";

        return ( false );
    }

    if ( _addon.manifest.api_version != g_engineApiVersion ) {
        _addon.error = "Unsupported api_version";

        return ( false );
    }

    _addon.dependencies.clear();

    for ( const auto& l_depText : _addon.manifest.dependencies ) {
        auto l_dep = _parseDependencyString( l_depText, false );

        if ( !l_dep || l_dep->name.empty() ) {
            _addon.error = "Invalid dependency";

            return ( false );
        }

        _addon.dependencies.push_back( *l_dep );
    }

    for ( const auto& l_depText : _addon.manifest.optional_dependencies ) {
        auto l_dep = _parseDependencyString( l_depText, true );

        if ( !l_dep || l_dep->name.empty() ) {
            _addon.error = "Invalid optional dependency";

            return ( false );
        }

        _addon.dependencies.push_back( *l_dep );
    }

    _addon.fingerprint = _computeFingerprint( _addon.root );

    return ( true );
}

auto AddonLoaderT::_loadPatchManifest( addonRecordT& _addon ) -> bool {
    const auto l_patchPath = _addon.root / "patch.json";

    if ( !std::filesystem::exists( l_patchPath ) ) {
        return ( true );
    }

    std::string l_text{};

    if ( !_readTextFile( l_patchPath, l_text ) ) {
        _addon.error = "Failed to read patch.json";

        return ( false );
    }

    auto l_ec = glz::read_json( _addon.patches, l_text );

    if ( l_ec ) {
        _addon.error = "Invalid patch.json";

        return ( false );
    }

    for ( const auto& l_patch : _addon.patches ) {
        const bool l_hasPattern = l_patch.pattern.has_value();

        if ( !_validateBytesString( l_patch.bytes ) ) {
            _addon.error = "Invalid patch bytes";

            return ( false );
        }

        if ( !_parseHexU64( l_patch.address ) ) {
            _addon.error = "Invalid patch address";

            return ( false );
        }

        if ( l_hasPattern ) {
            if ( !_validatePatternString( *l_patch.pattern ) ) {
                _addon.error = "Invalid patch pattern";

                return ( false );
            }
        }
    }

    return ( true );
}

auto AddonLoaderT::scan() -> bool {
    _mAddons.clear();
    _mLoadOrder.clear();
    _mReverseDeps.clear();

    std::error_code l_ec{};

    if ( !std::filesystem::exists( _mAddonsRoot, l_ec ) ) {
        return ( true );
    }

    for ( const auto& l_entry :
          std::filesystem::directory_iterator( _mAddonsRoot ) ) {
        if ( !l_entry.is_directory() ) {
            continue;
        }

        const auto& l_path = l_entry.path();

        if ( !_safeUnderRoot( _mAddonsRoot, l_path ) ) {
            continue;
        }

        const auto l_id = l_path.filename().string();

        if ( !_validateSafeName( l_id ) ) {
            continue;
        }

        addonRecordT l_addon = {
            .id = l_id,
            .root = std::filesystem::weakly_canonical( l_path, l_ec ),
        };

        if ( l_ec ) {
            continue;
        }

        if ( !_loadManifest( l_addon ) ) {
            _mAddons.emplace( l_id, std::move( l_addon ) );

            continue;
        }

        if ( !_loadPatchManifest( l_addon ) ) {
            _mAddons.emplace( l_id, std::move( l_addon ) );

            continue;
        }

        l_addon.runtime = _createRuntime( l_addon );

        if ( !l_addon.runtime ) {
            l_addon.error = "Unsupported runtime";
        }

        _mAddons.emplace( l_id, std::move( l_addon ) );
    }

    return ( _buildGraph() && _topoSort() );
}

auto AddonLoaderT::_buildGraph() -> bool {
    _mReverseDeps.clear();

    for ( const auto& [ l_id, l_addon ] : _mAddons ) {
        ( void )l_addon;
        _mReverseDeps.try_emplace( l_id );
    }

    for ( const auto& [ l_id, l_addon ] : _mAddons ) {
        if ( !l_addon.error.empty() ) {
            continue;
        }

        for ( const auto& l_dep : l_addon.dependencies ) {
            auto l_it = _mAddons.find( l_dep.name );

            if ( l_it == _mAddons.end() ) {
                if ( l_dep.optional ) {
                    continue;
                }

                auto& l_mut = _mAddons.at( l_id );

                l_mut.error = "Missing dependency: " + l_dep.name;

                continue;
            }

            auto l_depVersion = parseVersion( l_it->second.manifest.version );

            if ( !l_depVersion ) {
                auto& l_mut = _mAddons.at( l_id );

                l_mut.error = "Invalid dependency version: " + l_dep.name;

                continue;
            }

            if ( !satisfies( *l_depVersion, l_dep.range ) ) {
                if ( l_dep.optional ) {
                    continue;
                }

                auto& l_mut = _mAddons.at( l_id );

                l_mut.error = "Dependency version mismatch: " + l_dep.name;

                continue;
            }

            _mReverseDeps[ l_dep.name ].push_back( l_id );
        }
    }

    for ( const auto& [ l_id, l_addon ] : _mAddons ) {
        if ( l_addon.error.empty() ) {
            continue;
        }

        for ( const auto& l_dep : l_addon.dependencies ) {
            auto l_revIt = _mReverseDeps.find( l_dep.name );

            if ( l_revIt != _mReverseDeps.end() ) {
                auto& l_vec = l_revIt->second;

                auto l_sub =
                    std::ranges::remove( l_vec.begin(), l_vec.end(), l_id );

                l_vec.erase( l_sub.begin(), l_sub.end() );
            }
        }
    }

    return ( true );
}

auto AddonLoaderT::_topoSort() -> bool {
    std::unordered_map< std::string, std::size_t > l_indegree{};

    for ( const auto& [ l_id, l_addon ] : _mAddons ) {
        if ( !l_addon.error.empty() ) {
            continue;
        }

        l_indegree.try_emplace( l_id, 0 );
    }

    for ( const auto& [ l_id, l_addon ] : _mAddons ) {
        if ( !l_addon.error.empty() ) {
            continue;
        }

        for ( const auto& l_dep : l_addon.dependencies ) {
            auto l_it = _mAddons.find( l_dep.name );

            if ( l_it == _mAddons.end() || !l_it->second.error.empty() ) {
                continue;
            }

            if ( !l_indegree.contains( l_dep.name ) ) {
                continue;
            }

            ++l_indegree[ l_id ];
        }
    }

    std::queue< std::string > l_queue{};

    for ( const auto& [ l_id, l_deg ] : l_indegree ) {
        if ( l_deg == 0 ) {
            l_queue.push( l_id );
        }
    }

    _mLoadOrder.clear();

    while ( !l_queue.empty() ) {
        auto l_id = l_queue.front();

        l_queue.pop();

        _mLoadOrder.push_back( l_id );

        auto l_revIt = _mReverseDeps.find( l_id );

        if ( l_revIt == _mReverseDeps.end() ) {
            continue;
        }

        for ( const auto& l_child : l_revIt->second ) {
            auto l_degIt = l_indegree.find( l_child );

            if ( l_degIt == l_indegree.end() ) {
                continue;
            }

            if ( l_degIt->second > 0 ) {
                --l_degIt->second;

                if ( l_degIt->second == 0 ) {
                    l_queue.push( l_child );
                }
            }
        }
    }

    const size_t l_expected =
        std::ranges::count_if( _mAddons, []( const auto& _pair ) -> bool {
            return ( _pair.second.error.empty() );
        } );

    if ( _mLoadOrder.size() != l_expected ) {
        for ( auto& [ l_id, l_addon ] : _mAddons ) {
            if ( l_addon.error.empty() ) {
                l_addon.error = "Circular dependency detected";
            }
        }

        return ( false );
    }

    return ( true );
}

auto AddonLoaderT::_canLoadAddon( const addonRecordT& _addon ) const -> bool {
    return ( _addon.error.empty() && !_addon.loaded &&
             _addon.runtime != nullptr );
}

auto AddonLoaderT::_createRuntime( const addonRecordT& _addon )
    -> std::unique_ptr< addonRecordT::runtimeT > {
    if ( _addon.manifest.runtime == "lua" ) {
        return ( std::make_unique< LuaRuntimeT >() );
    }

    if ( _addon.manifest.runtime == "wasm" ) {
        return ( std::make_unique< WasmRuntimeT >() );
    }

    return ( nullptr );
}

auto AddonLoaderT::_computeFingerprint(
    const std::filesystem::path& _root ) const -> std::uint64_t {
    std::uint64_t l_hash = 0;

    auto l_feed = [ & ]( const void* _data, std::size_t _size ) -> void {
        l_hash ^=
            stdfunc::hash::balanced< uint64_t >( std::span< const std::byte >{
                std::bit_cast< std::byte* >( _data ), _size } );
    };

    std::error_code l_ec{};

    for ( const auto& l_entry :
          std::filesystem::recursive_directory_iterator( _root, l_ec ) ) {
        if ( l_ec ) {
            break;
        }

        if ( !l_entry.is_regular_file() ) {
            continue;
        }

        const auto l_rel =
            std::filesystem::relative( l_entry.path(), _root, l_ec );

        if ( l_ec ) {
            continue;
        }

        const auto l_relStr = l_rel.string();

        l_feed( l_relStr.data(), l_relStr.size() );

        std::ifstream l_ifs( l_entry.path(), std::ios::binary );

        if ( !l_ifs ) {
            continue;
        }

        std::array< char, 4096 > l_buf{};

        while ( l_ifs ) {
            l_ifs.read( l_buf.data(),
                        static_cast< std::streamsize >( l_buf.size() ) );

            const auto l_got = static_cast< std::size_t >( l_ifs.gcount() );

            if ( l_got != 0 ) {
                l_feed( l_buf.data(), l_got );
            }
        }
    }

    return ( l_hash );
}

auto AddonLoaderT::loadAll() -> bool {
    for ( const auto& l_id : _mLoadOrder ) {
        auto& l_addon = _mAddons.at( l_id );

        if ( !_canLoadAddon( l_addon ) ) {
            continue;
        }

        if ( !_applyPatches( l_addon ) ) {
            continue;
        }

        if ( !l_addon.runtime->load( l_addon ) ) {
            l_addon.error = "Runtime load failed";

            continue;
        }

        l_addon.loaded = true;
    }

    return ( true );
}

auto AddonLoaderT::_unloadDependentsFirst( const std::string& _id ) -> void {
    auto l_it = _mReverseDeps.find( _id );

    if ( l_it == _mReverseDeps.end() ) {
        return;
    }

    for ( const auto& l_child : l_it->second ) {
        auto& l_addon = _mAddons.at( l_child );

        if ( l_addon.loaded && l_addon.runtime ) {
            l_addon.runtime->unload( l_addon );

            l_addon.loaded = false;
        }

        _unloadDependentsFirst( l_child );
    }
}

auto AddonLoaderT::unloadAll() -> void {
    for ( auto& l_it : std::views::reverse( _mLoadOrder ) ) {
        auto& l_addon = _mAddons.at( l_it );

        if ( l_addon.loaded && l_addon.runtime ) {
            l_addon.runtime->unload( l_addon );

            l_addon.loaded = false;
        }

        l_addon.applied_patches.clear();
    }

    _mUsedPatchAddresses.clear();
    _mUsedPatchPatterns.clear();
}

auto AddonLoaderT::_reloadAll() -> void {
    unloadAll();
    scan();
    loadAll();
}

auto AddonLoaderT::requestReload() -> void {
    _mReloadRequested = true;
}

auto AddonLoaderT::dispatchEvent( std::string_view _event ) -> void {
    for ( const auto& l_id : _mLoadOrder ) {
        auto& l_addon = _mAddons.at( l_id );

        if ( !l_addon.loaded || !l_addon.runtime ) {
            continue;
        }

        // Only call addons that declared this event.
        if ( std::ranges::find( l_addon.manifest.events, _event ) ==
             l_addon.manifest.events.end() ) {
            continue;
        }

        l_addon.runtime->callEvent( l_addon, _event );
    }
}

// auto addon_loader_t::tick( bool _blocking ) -> void {
//     if ( m_watch ) {
//         m_watch->check( _blocking );
//     }

//     if ( m_reloadRequested ) {
//         m_reloadRequested = false;
//         _reloadAll();
//     }
// }

} // namespace mod
