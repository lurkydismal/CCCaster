#pragma once

#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "api.hpp"

namespace mod {

struct versionT {
    int major{};
    int minor{};
    int patch{};
};

struct versionRangeT {
    enum class kindT : std::uint8_t {
        any,
        exact,
        caret,
        greaterEqual,
        greater,
        lessEqual,
        less,
    };

    kindT kind{ kindT::any };
    versionT value{};
};

struct dependencyT {
    std::string name{};
    versionRangeT range{};
    bool optional{ false };
};

struct addonManifestT {
    std::string author{};
    std::string version{};
    std::string runtime{};
    std::string entry{};
    std::vector< std::string > dependencies{};
    std::vector< std::string > optional_dependencies{};
    std::vector< std::string > events{};
    std::uint32_t api_version{};
};

struct patchActionT {
    std::string address{}; // hex string like "0x123"
    std::optional< std::string >
        pattern{};       // pattern string like "CC ? DD ?? FF"
    std::string bytes{}; // hex byte string like "AA BB"
};

struct addonRecordT {
    std::string id{};
    std::filesystem::path root{};
    addonManifestT manifest{};
    std::vector< dependencyT > dependencies{};
    std::vector< patchActionT > patches{};
    bool loadable{ false };
    std::string error{};
    std::uint64_t fingerprint{};
    std::vector< wrapper::patch_t > applied_patches{};

    struct runtimeT {
        runtimeT() = default;
        virtual ~runtimeT() = default;

        runtimeT( const runtimeT& ) = default;
        runtimeT( runtimeT&& ) = delete;
        auto operator=( const runtimeT& ) -> runtimeT& = default;
        auto operator=( runtimeT&& ) -> runtimeT& = delete;

        virtual auto load( addonRecordT& _addon ) -> bool = 0;
        virtual void unload( addonRecordT& _addon ) = 0;
        virtual auto callEvent( addonRecordT& _addon, std::string_view _event )
            -> bool = 0;
    };

    std::unique_ptr< runtimeT > runtime{};
    bool loaded{ false };
};

} // namespace mod

template <>
struct std::formatter< mod::addonManifestT > {
    constexpr auto parse( std::format_parse_context& _ctx ) {
        return _ctx.begin();
    }

    auto format( const mod::addonManifestT& _m,
                 std::format_context& _ctx ) const {
        auto l_join =
            []( const std::vector< std::string >& _v ) -> std::string {
            std::string l_out = "[";

            for ( size_t l_i = 0; l_i < _v.size(); ++l_i ) {
                l_out += std::format( "\"{}\"", _v[ l_i ] );
                if ( l_i + 1 < _v.size() )
                    l_out += ", ";
            }

            l_out += "]";

            return ( l_out );
        };

        return std::format_to( _ctx.out(),
                               "addon_manifest_t{{\n"
                               "  author: \"{}\",\n"
                               "  version: \"{}\",\n"
                               "  runtime: \"{}\",\n"
                               "  entry: \"{}\",\n"
                               "  dependencies: {},\n"
                               "  optional_dependencies: {},\n"
                               "  events: {},\n"
                               "  api_version: {}\n"
                               "}}",
                               _m.author, _m.version, _m.runtime, _m.entry,
                               l_join( _m.dependencies ),
                               l_join( _m.optional_dependencies ),
                               l_join( _m.events ), _m.api_version );
    }
};

template <>
struct std::formatter< mod::addonRecordT > {
    constexpr auto parse( std::format_parse_context& _ctx ) {
        return _ctx.begin();
    }

    auto format( const mod::addonRecordT& _a,
                 std::format_context& _ctx ) const {
        return std::format_to( _ctx.out(),
                               "addon_record_t{{\n"
                               "  id: \"{}\",\n"
                               "  root: \"{}\",\n"
                               "  manifest: <{}>,\n"
                               "  dependencies: {} items,\n"
                               "  patches: {} items,\n"
                               "  loadable: {},\n"
                               "  error: \"{}\",\n"
                               "  fingerprint: {:#X},\n"
                               "  runtime: {},\n"
                               "  loaded: {}\n"
                               "}}",
                               _a.id, _a.root.string(),
                               _a.manifest, // or customize if printable
                               _a.dependencies.size(), _a.patches.size(),
                               _a.loadable, _a.error, _a.fingerprint,
                               _a.runtime ? "present" : "null", _a.loaded );
    }
};
