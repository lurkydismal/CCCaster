#pragma once

#include <format>
#include <iostream>
#include <mutex>

namespace logg {

using level_t = enum class level : uint8_t {
    trace,
    debug,
    info,
    warning,
    error,
};

namespace {

inline std::mutex g_mutex;

inline auto log( level_t _level, const std::string& _message ) -> void {
    std::lock_guard l_lock( g_mutex );

    switch ( _level ) {
        case ( level_t::trace ): {
            std::cout << "[trace] ";

            break;
        }

        case ( level_t::debug ): {
            std::cout << "[debug] ";

            break;
        }

        case ( level_t::info ): {
            std::cout << "[info ] ";

            break;
        }

        case ( level_t::warning ): {
            std::cerr << "[warn ] ";

            break;
        }

        case ( level_t::error ): {
            std::cerr << "[error] ";

            break;
        }

        default: {
            std::terminate();
        }
    }

    std::cout << _message << '\n';
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
