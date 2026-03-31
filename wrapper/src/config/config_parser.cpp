#include "wrapper/config_parser.hpp"

#include <format>
#include <limits>
#include <optional>
#include <string_view>

#include "logg.hpp"

template <>
struct std::formatter< wrappercfg::data_t > {
    constexpr auto parse( std::format_parse_context& _ctx ) {
        return _ctx.begin();
    }

    auto format( const wrappercfg::data_t& _data,
                 std::format_context& _ctx ) const {
        std::string_view l_view{ _data.value, _data.size };

        return std::format_to( _ctx.out(),
                               "wrappercfg::data_t{{ size={}, value='{}' }}",
                               _data.size, l_view );
    }
};

namespace {

namespace json_cfg {

void skipWs( std::string_view _s, size_t& _i ) {
    while ( _i < _s.size() ) {
        auto const l_c = static_cast< unsigned char >( _s[ _i ] );
        if ( ( l_c != ' ' ) && ( l_c != '\t' ) && ( l_c != '\n' ) &&
             ( l_c != '\r' ) ) {
            break;
        }
        ++_i;
    }
}

auto consume( std::string_view _s, size_t& _i, char _ch ) -> bool {
    skipWs( _s, _i );
    if ( ( _i >= _s.size() ) || ( _s[ _i ] != _ch ) ) {
        return ( false );
    }
    ++_i;
    return ( true );
}

auto parseString( std::string_view _s, size_t& _i, std::string_view& _out )
    -> bool {
    skipWs( _s, _i );
    if ( ( _i >= _s.size() ) || ( _s[ _i ] != '"' ) ) {
        return ( false );
    }

    ++_i;
    size_t const l_begin = _i;

    while ( _i < _s.size() ) {
        char const l_c = _s[ _i ];
        if ( l_c == '"' ) {
            _out = _s.substr( l_begin, ( _i - l_begin ) );
            ++_i;
            return ( true );
        }
        if ( l_c == '\\' ) {
            return ( false );
        }
        ++_i;
    }

    return ( false );
}

auto parseBool( std::string_view _s, size_t& _i, bool& _out ) -> bool {
    skipWs( _s, _i );

    if ( _s.substr( _i, 4 ) == "true" ) {
        _out = true;
        _i += 4;
        return ( true );
    }

    if ( _s.substr( _i, 5 ) == "false" ) {
        _out = false;
        _i += 5;
        return ( true );
    }

    return ( false );
}

auto parseUint8( std::string_view _s, size_t& _i, uint8_t& _out ) -> bool {
    skipWs( _s, _i );
    if ( _i >= _s.size() ) {
        return ( false );
    }

    uint32_t l_value = 0;
    size_t l_start = _i;

    while ( _i < _s.size() ) {
        auto const l_c = static_cast< unsigned char >( _s[ _i ] );
        if ( ( l_c < '0' ) || ( l_c > '9' ) ) {
            break;
        }

        l_value = ( l_value * 10u ) + static_cast< uint32_t >( l_c - '0' );
        if ( l_value > static_cast< uint32_t >(
                           std::numeric_limits< uint8_t >::max() ) ) {
            return ( false );
        }
        ++_i;
    }

    if ( _i == l_start ) {
        return ( false );
    }

    _out = static_cast< uint8_t >( l_value );
    return ( true );
}

auto skipValue( std::string_view _s, size_t& _i ) -> bool {
    skipWs( _s, _i );

    if ( _i >= _s.size() ) {
        return ( false );
    }

    switch ( _s[ _i ] ) {
        case '"': {
            ++_i;
            while ( _i < _s.size() ) {
                if ( _s[ _i ] == '\\' ) {
                    if ( ( _i + 1 ) >= _s.size() ) {
                        return ( false );
                    }
                    _i += 2;
                    continue;
                }
                if ( _s[ _i ] == '"' ) {
                    ++_i;
                    return ( true );
                }
                ++_i;
            }
            return ( false );
        }

        case '{': {
            ++_i;
            skipWs( _s, _i );
            if ( consume( _s, _i, '}' ) ) {
                return ( true );
            }

            while ( true ) {
                std::string_view l_key{};
                if ( !parseString( _s, _i, l_key ) || !consume( _s, _i, ':' ) ||
                     !skipValue( _s, _i ) ) {
                    return ( false );
                }

                skipWs( _s, _i );
                if ( consume( _s, _i, '}' ) ) {
                    return ( true );
                }
                if ( !consume( _s, _i, ',' ) ) {
                    return ( false );
                }
            }
        }

        case '[': {
            ++_i;
            skipWs( _s, _i );

            if ( consume( _s, _i, ']' ) ) {
                return ( true );
            }

            while ( true ) {
                if ( !skipValue( _s, _i ) ) {
                    return ( false );
                }
                skipWs( _s, _i );
                if ( consume( _s, _i, ']' ) ) {
                    return ( true );
                }
                if ( !consume( _s, _i, ',' ) ) {
                    return ( false );
                }
            }
        }

        case 't':
            if ( _s.substr( _i, 4 ) == "true" ) {
                _i += 4;
                return ( true );
            }
            return ( false );

        case 'f':
            if ( _s.substr( _i, 5 ) == "false" ) {
                _i += 5;
                return ( true );
            }
            return ( false );

        case 'n':
            if ( _s.substr( _i, 4 ) == "null" ) {
                _i += 4;
                return ( true );
            }
            return ( false );

        default: {
            size_t const l_begin = _i;
            while ( _i < _s.size() ) {
                char const l_c = _s[ _i ];
                if ( ( l_c == ' ' ) || ( l_c == '\t' ) || ( l_c == '\n' ) ||
                     ( l_c == '\r' ) || ( l_c == ',' ) || ( l_c == '}' ) ||
                     ( l_c == ']' ) ) {
                    break;
                }
                ++_i;
            }

            return ( _i > l_begin );
        }
    }
}

auto parseValueForKey( std::string_view _key,
                       std::string_view _s,
                       size_t& _i,
                       wrappercfg::wrapperData_t& _out ) -> bool {
    if ( _key == "verbose" ) {
        return ( parseUint8( _s, _i, _out.verbose ) );
    }
    if ( _key == "trace" ) {
        return ( parseBool( _s, _i, _out.trace ) );
    }
    if ( _key == "timings" ) {
        return ( parseBool( _s, _i, _out.timings ) );
    }
    if ( _key == "no_patches" ) {
        return ( parseBool( _s, _i, _out.no_patches ) );
    }

    logg::debug( "parseValueForKey: unknown key '{}', skipping", _key );
    return ( skipValue( _s, _i ) );
}

} // namespace json_cfg

} // namespace

namespace wrappercfg {

auto parseWrapperData( std::string_view _json )
    -> std::optional< wrapperData_t > {
    size_t l_i = 0;
    wrapperData_t l_out{};

    if ( !json_cfg::consume( _json, l_i, '{' ) ) {
        return ( std::nullopt );
    }

    json_cfg::skipWs( _json, l_i );
    if ( json_cfg::consume( _json, l_i, '}' ) ) {
        return ( l_out );
    }

    while ( true ) {
        std::string_view l_key{};

        if ( !json_cfg::parseString( _json, l_i, l_key ) ) {
            return ( std::nullopt );
        }

        if ( !json_cfg::consume( _json, l_i, ':' ) ) {
            return ( std::nullopt );
        }

        if ( !json_cfg::parseValueForKey( l_key, _json, l_i, l_out ) ) {
            return ( std::nullopt );
        }

        json_cfg::skipWs( _json, l_i );
        if ( json_cfg::consume( _json, l_i, '}' ) ) {
            break;
        }

        if ( !json_cfg::consume( _json, l_i, ',' ) ) {
            return ( std::nullopt );
        }
    }

    json_cfg::skipWs( _json, l_i );
    if ( l_i != _json.size() ) {
        return ( std::nullopt );
    }

    return ( l_out );
}

auto parseWrapperData( const data_t* _data ) -> std::optional< wrapperData_t > {
    if ( _data == nullptr ) {
        logg::error( "Shared data is null" );
        return ( std::nullopt );
    }

    const std::string_view l_json{ _data->value, _data->size };
    const auto l_cfg = parseWrapperData( l_json );

    if ( !l_cfg ) {
        logg::warning( "parseWrapperData(env): JSON parse failed" );
        return ( std::nullopt );
    }

    bool l_verboseSet = true;
    bool l_traceSet = true;

    if ( l_cfg->verbose ) {
        l_verboseSet =
            logg::setLogLevel( static_cast< logg::level_t >( l_cfg->verbose ) );
    }

    if ( l_cfg->trace ) {
        l_traceSet = logg::setLogLevel( logg::level_t::trace );
    }

    if ( !l_traceSet || !l_verboseSet ) {
        logg::warning(
            "Config logg update incomplete: trace_set={}, verbose_set={}",
            l_traceSet, l_verboseSet );

    } else {
        logg::trace( "config set: trace={}, verbose={}", l_cfg->trace,
                     l_cfg->verbose );
    }

    return ( l_cfg );
}

} // namespace wrappercfg
