#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <utility>

namespace logg {

using level_t = enum class level : uint8_t {
    trace,
    debug,
    info,
    warning,
    error,
};

namespace detail {

auto normalizeFlag( const char* _value ) -> std::string;

auto isTruthy( const char* _value ) -> bool;

auto isDebugEnabled() -> bool;

auto isTraceEnabled() -> bool;

auto tryGetLogPath() -> const std::string*;

auto log( level_t _level, const std::string& _message ) -> void;

} // namespace detail

template < typename... Args >
auto trace( std::format_string< Args... > _format, Args&&... _arguments )
    -> void {
    detail::log(
        level_t::trace,
        std::format( _format, std::forward< Args >( _arguments )... ) );
}

template < typename... Args >
auto debug( std::format_string< Args... > _format, Args&&... _arguments )
    -> void {
    detail::log(
        level_t::debug,
        std::format( _format, std::forward< Args >( _arguments )... ) );
}

template < typename... Args >
auto info( std::format_string< Args... > _format, Args&&... _arguments )
    -> void {
    detail::log(
        level_t::info,
        std::format( _format, std::forward< Args >( _arguments )... ) );
}

template < typename... Args >
auto warning( std::format_string< Args... > _format, Args&&... _arguments )
    -> void {
    detail::log(
        level_t::warning,
        std::format( _format, std::forward< Args >( _arguments )... ) );
}

template < typename... Args >
auto error( std::format_string< Args... > _format, Args&&... _arguments )
    -> void {
    detail::log(
        level_t::error,
        std::format( _format, std::forward< Args >( _arguments )... ) );
}

} // namespace logg
