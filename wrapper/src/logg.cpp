#include "logg.hpp"

#include <array>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <type_traits>

namespace {

std::mutex g_mutex;

std::atomic< logg::level_t > g_enabledLogLevel{ logg::level_t::error };

} // namespace

namespace logg {

namespace detail {

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

constexpr auto checkLogLevel( level_t _level ) {
    return ( g_enabledLogLevel >= _level );
}

auto log( level_t _level, const std::string& _message ) -> void {
    static_assert( static_cast< std::underlying_type_t< level_t > >(
                       level_t::trace ) == 4 );

    if ( !checkLogLevel( _level ) ) {
        return;
    }

    static constexpr std::array l_prefixes = {
        "[ERROR]", "[WARN]", "[INFO]", "[DEBUG]", "[TRACE]",
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

auto setLogLevel( level_t _level ) -> bool {
    if ( ( _level < level_t::error ) || ( _level > level_t::trace ) ) {
        return ( false );
    }

    g_enabledLogLevel = _level;

    return ( true );
}

} // namespace logg
