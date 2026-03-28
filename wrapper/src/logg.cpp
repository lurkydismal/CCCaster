#include "logg.hpp"

#include <array>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>
#include <string>
#include <type_traits>

namespace logg {

std::mutex g_mutex;

namespace detail {

auto normalizeFlag( const char* _value ) -> std::string {
    if ( _value == nullptr ) {
        return ( "" );
    }

    std::string l_value = _value;

    for ( char& l_char : l_value ) {
        l_char = static_cast< char >(
            std::tolower( static_cast< unsigned char >( l_char ) ) );
    }

    return ( l_value );
}

auto isTruthy( const char* _value ) -> bool {
    const std::string l_value = normalizeFlag( _value );

    return ( l_value == "1" ) || ( l_value == "true" ) || ( l_value == "ok" ) ||
           ( l_value == "yes" );
}

auto getVerboseLevel() -> uint8_t {
    const char* l_env = std::getenv( "WRAPPER_VERBOSE" );

    if ( l_env == nullptr ) {
        return ( 0 );
    }

    const std::string_view l_level{ l_env };
    unsigned l_verboseLevel = 0;
    const auto [ l_ptr, l_error ] =
        std::from_chars( l_level.begin(), l_level.end(), l_verboseLevel );

    if ( ( l_error != std::errc() ) || ( l_ptr != l_level.end() ) ||
         ( l_verboseLevel > std::numeric_limits< uint8_t >::max() ) ) {
        return ( 0 );
    }

    return ( static_cast< uint8_t >( l_verboseLevel ) );
}

auto isDebugEnabled() -> bool {
    const bool l_debugEnabled = isTruthy( std::getenv( "WRAPPER_DEBUG" ) ) ||
                                isTruthy( std::getenv( "WRAPPER_TRACE" ) );

    return ( l_debugEnabled );
}

auto isTraceEnabled() -> bool {
    const bool l_traceEnabled = isTruthy( std::getenv( "WRAPPER_TRACE" ) );

    return ( l_traceEnabled );
}

auto tryGetLogPath() -> const std::string* {
    static const std::string l_logPath = []() -> std::string {
        const char* l_env = std::getenv( "WRAPPER_LOG" );

        constexpr size_t l_logFilenameMaxLength = 100;

        if ( ( l_env == nullptr ) ||
             ( strnlen( l_env, l_logFilenameMaxLength ) ==
               l_logFilenameMaxLength ) ) {
            return {};
        }

        return ( l_env );
    }();

    if ( l_logPath.empty() ) {
        return ( nullptr );
    }

    return ( &l_logPath );
}

auto log( level_t _level, const std::string& _message ) -> void {
    static_assert( static_cast< std::underlying_type_t< level_t > >(
                       level_t::error ) == 4 );

    {
        bool l_allow = false;

        if ( _level == level_t::trace ) {
            l_allow = isTraceEnabled();

        } else if ( _level == level_t::debug ) {
            l_allow = isDebugEnabled() || isTraceEnabled() ||
                      ( getVerboseLevel() >= 3 );

        } else if ( _level == level_t::info ) {
            l_allow = isDebugEnabled() || isTraceEnabled() ||
                      ( getVerboseLevel() >= 2 );

        } else if ( _level == level_t::warning ) {
            l_allow = isDebugEnabled() || isTraceEnabled() ||
                      ( getVerboseLevel() >= 1 );

        } else if ( _level == level_t::error ) {
            l_allow = true;
        }

        if ( !l_allow ) {
            return;
        }
    }

    static constexpr std::array l_prefixes = {
        "[TRACE]", "[DEBUG]", "[INFO]", "[WARN]", "[ERROR]",
    };

    const bool l_isError =
        ( _level == level_t::warning ) || ( _level == level_t::error );

    auto& l_stream = l_isError ? std::cerr : std::cout;
    const std::string l_line = std::format(
        "[WRAPPER] {} {}",
        l_prefixes.at(
            static_cast< std::underlying_type_t< level_t > >( _level ) ),
        _message );

    std::lock_guard l_lock( g_mutex );

    if ( const std::string* l_logPath = tryGetLogPath();
         l_logPath != nullptr ) {
        std::ofstream l_logFile( *l_logPath, std::ios::out | std::ios::app );

        if ( l_logFile.is_open() ) {
            l_logFile << l_line << '\n';
            l_logFile.flush();
        }

    } else {
        l_stream << l_line << '\n';
    }
}

} // namespace detail

} // namespace logg
