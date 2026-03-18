#pragma once

#include <charconv>
#include <string_view>

#include "addon_types.hpp"

namespace mod {

inline auto parseVersion( std::string_view _text )
    -> std::optional< versionT > {
    versionT l_out{};

    auto l_nextComponent = [ & ]( std::string_view& _sv, int& _dst ) -> bool {
        while ( !_sv.empty() &&
                ( _sv.front() == ' ' || _sv.front() == '\t' ) ) {
            _sv.remove_prefix( 1 );
        }

        if ( _sv.empty() ) {
            return ( false );
        }

        int l_value = 0;
        const char* l_begin = _sv.begin();
        const char* l_end = _sv.end();

        auto l_res = std::from_chars( l_begin, l_end, l_value );

        if ( l_res.ec != std::errc{} ) {
            return ( false );
        }

        _dst = l_value;
        _sv.remove_prefix( static_cast< std::size_t >( l_res.ptr - l_begin ) );

        if ( !_sv.empty() && _sv.front() == '.' ) {
            _sv.remove_prefix( 1 );
        }

        return ( true );
    };

    std::string_view l_work = _text;

    if ( !l_nextComponent( l_work, l_out.major ) ) {
        return ( std::nullopt );
    }

    if ( !l_work.empty() ) {
        if ( !l_nextComponent( l_work, l_out.minor ) ) {
            return ( std::nullopt );
        }
    }

    if ( !l_work.empty() ) {
        if ( !l_nextComponent( l_work, l_out.patch ) ) {
            return ( std::nullopt );
        }
    }

    return ( l_out );
}

inline auto compareVersion( const versionT& _lhs, const versionT& _rhs )
    -> int {
    if ( _lhs.major != _rhs.major ) {
        return ( ( _lhs.major < _rhs.major ) ? -1 : 1 );

    } else if ( _lhs.minor != _rhs.minor ) {
        return ( ( _lhs.minor < _rhs.minor ) ? -1 : 1 );

    } else if ( _lhs.patch != _rhs.patch ) {
        return ( ( _lhs.patch < _rhs.patch ) ? -1 : 1 );

    } else {
        return ( 0 );
    }
}

inline auto parseVersionRange( std::string_view _text )
    -> std::optional< versionRangeT > {
    versionRangeT l_out{};

    while ( !_text.empty() &&
            ( _text.front() == ' ' || _text.front() == '\t' ) ) {
        _text.remove_prefix( 1 );
    }

    while ( !_text.empty() &&
            ( _text.back() == ' ' || _text.back() == '\t' ) ) {
        _text.remove_suffix( 1 );
    }

    if ( _text.empty() ) {
        return ( l_out );
    }

    auto l_trim = []( std::string_view& _sv ) -> void {
        while ( !_sv.empty() &&
                ( _sv.front() == ' ' || _sv.front() == '\t' ) ) {
            _sv.remove_prefix( 1 );
        }

        while ( !_sv.empty() && ( _sv.back() == ' ' || _sv.back() == '\t' ) ) {
            _sv.remove_suffix( 1 );
        }
    };

    if ( _text.front() == '^' ) {
        auto l_version = parseVersion( _text.substr( 1 ) );

        if ( !l_version ) {
            return ( std::nullopt );
        }

        l_out.kind = versionRangeT::kindT::caret;
        l_out.value = *l_version;

        return ( l_out );
    }

    if ( _text.starts_with( ">=" ) || _text.starts_with( "<=" ) ||
         _text.starts_with( "==" ) ) {
        auto l_op = _text.substr( 0, 2 );
        auto l_rest = _text.substr( 2 );
        l_trim( l_rest );

        if ( l_rest.starts_with( "^" ) ) {
            l_rest.remove_prefix( 1 );
        }

        auto l_version = parseVersion( l_rest );

        if ( !l_version ) {
            return ( std::nullopt );
        }

        if ( l_op == ">=" ) {
            l_out.kind = versionRangeT::kindT::greaterEqual;

        } else if ( l_op == "<=" ) {
            l_out.kind = versionRangeT::kindT::lessEqual;

        } else {
            l_out.kind = versionRangeT::kindT::exact;
        }

        l_out.value = *l_version;

        return ( l_out );
    }

    if ( _text.front() == '>' || _text.front() == '<' ||
         _text.front() == '=' ) {
        const char l_op = _text.front();

        auto l_rest = _text.substr( 1 );

        l_trim( l_rest );

        if ( l_rest.starts_with( "^" ) ) {
            l_rest.remove_prefix( 1 );
        }

        auto l_version = parseVersion( l_rest );

        if ( !l_version ) {
            return ( std::nullopt );
        }

        switch ( l_op ) {
            case '>': {
                l_out.kind = versionRangeT::kindT::greater;

                break;
            }

            case '<': {
                l_out.kind = versionRangeT::kindT::less;

                break;
            }

            default: {
                l_out.kind = versionRangeT::kindT::exact;
            }
        }

        l_out.value = *l_version;

        return ( l_out );
    }

    if ( _text.starts_with( "^" ) ) {
        auto l_version = parseVersion( _text.substr( 1 ) );

        if ( !l_version ) {
            return ( std::nullopt );
        }

        l_out.kind = versionRangeT::kindT::caret;
        l_out.value = *l_version;

        return ( l_out );
    }

    auto l_version = parseVersion( _text );

    if ( !l_version ) {
        return ( std::nullopt );
    }

    l_out.kind = versionRangeT::kindT::exact;
    l_out.value = *l_version;

    return ( l_out );
}

inline auto satisfies( const versionT& _version, const versionRangeT& _range )
    -> bool {
    switch ( _range.kind ) {
        case versionRangeT::kindT::any: {
            return ( true );
        }

        case versionRangeT::kindT::exact: {
            return ( compareVersion( _version, _range.value ) == 0 );
        }

        case versionRangeT::kindT::greaterEqual: {
            return ( compareVersion( _version, _range.value ) >= 0 );
        }

        case versionRangeT::kindT::greater: {
            return ( compareVersion( _version, _range.value ) > 0 );
        }

        case versionRangeT::kindT::lessEqual: {
            return ( compareVersion( _version, _range.value ) <= 0 );
        }

        case versionRangeT::kindT::less: {
            return ( compareVersion( _version, _range.value ) < 0 );
        }

        case versionRangeT::kindT::caret: {
            if ( compareVersion( _version, _range.value ) < 0 ) {
                return ( false );
            }

            if ( _range.value.major > 0 ) {
                versionT l_upper{
                    .major = _range.value.major + 1, .minor = 0, .patch = 0 };
                return ( compareVersion( _version, l_upper ) < 0 );
            }

            if ( _range.value.minor > 0 ) {
                versionT l_upper{
                    .major = 0, .minor = _range.value.minor + 1, .patch = 0 };
                return ( compareVersion( _version, l_upper ) < 0 );
            }

            versionT l_upper{
                .major = 0, .minor = 0, .patch = _range.value.patch + 1 };
            return ( compareVersion( _version, l_upper ) < 0 );
        }
    }

    return ( false );
}

} // namespace mod
