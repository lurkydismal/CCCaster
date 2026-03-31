#pragma once

namespace wrapperruntime {

/**
 * Returns true when the environment string contains a known truthy token.
 */
auto isTruthy( const char* _value ) -> bool;

/**
 * Blocks startup until a debugger attaches when WRAPPER_WAIT_DEBUGGER is enabled.
 */
auto waitForDebuggerIfNeeded() -> void;

/**
 * Emits debug logs for wrapper-related environment variables.
 */
auto logEnabledEnvironmentVariables() -> void;

} // namespace wrapperruntime
