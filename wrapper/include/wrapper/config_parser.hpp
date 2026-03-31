#pragma once

#include <optional>
#include <string_view>

#include "wrapper/types.hpp"

namespace wrappercfg {

/**
 * Parses wrapper JSON configuration from a UTF-8 JSON string.
 */
auto parseWrapperData( std::string_view _json ) -> std::optional< wrapperData_t >;

/**
 * Parses wrapper JSON configuration from shared-memory payload.
 */
auto parseWrapperData( const data_t* _data ) -> std::optional< wrapperData_t >;

} // namespace wrappercfg
