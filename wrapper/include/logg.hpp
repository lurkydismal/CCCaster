#pragma once

#include <cstdint>
#include <format>
#include <iostream>
#include <mutex>
#include <type_traits>
#include <utility>

namespace logg {

extern std::mutex g_mutex;

using level_t = enum class level : uint8_t {
    trace,
    debug,
    info,
    warning,
    error,
};

namespace {

inline auto log( level_t _level, const std::string& _message ) -> void {
    static_assert( static_cast< std::underlying_type_t< level_t > >(
                       level_t::error ) == 4 );

    static constexpr const std::array l_prefixes = {
        "[trace] ", "[debug] ", "[info ] ", "[warn ] ", "[error] ",
    };

    const bool l_isError =
        ( _level == level_t::warning ) || ( _level == level_t::error );

    auto& l_stream = l_isError ? std::cerr : std::cout;

    std::lock_guard l_lock( g_mutex );

    l_stream << l_prefixes.at(
                    static_cast< std::underlying_type_t< level_t > >( _level ) )
             << _message << '\n';
}

} // namespace

template < typename... Args >
auto trace( std::format_string< Args... > _format, Args&&... _arguments )
    -> void {
    log( level_t::trace,
         std::format( _format, std::forward< Args >( _arguments )... ) );
}

template < typename... Args >
auto debug( std::format_string< Args... > _format, Args&&... _arguments )
    -> void {
    log( level_t::debug,
         std::format( _format, std::forward< Args >( _arguments )... ) );
}

template < typename... Args >
auto info( std::format_string< Args... > _format, Args&&... _arguments )
    -> void {
    log( level_t::info,
         std::format( _format, std::forward< Args >( _arguments )... ) );
}

template < typename... Args >
auto warning( std::format_string< Args... > _format, Args&&... _arguments )
    -> void {
    log( level_t::warning,
         std::format( _format, std::forward< Args >( _arguments )... ) );
}

template < typename... Args >
auto error( std::format_string< Args... > _format, Args&&... _arguments )
    -> void {
    log( level_t::error,
         std::format( _format, std::forward< Args >( _arguments )... ) );
}

} // namespace logg
