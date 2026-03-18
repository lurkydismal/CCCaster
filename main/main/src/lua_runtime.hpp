#pragma once

#include <lua.hpp>

#include "addon_types.hpp"

namespace mod {

class LuaRuntimeT final : public addonRecordT::runtimeT {
public:
    explicit LuaRuntimeT() = default;
    ~LuaRuntimeT() override;

    LuaRuntimeT( const LuaRuntimeT& ) = default;
    LuaRuntimeT( LuaRuntimeT&& ) = delete;
    auto operator=( const LuaRuntimeT& ) -> LuaRuntimeT& = default;
    auto operator=( LuaRuntimeT&& ) -> LuaRuntimeT& = delete;

    auto load( addonRecordT& _addon ) -> bool override;
    void unload( addonRecordT& _addon ) override;
    auto callEvent( addonRecordT& _addon, std::string_view _event )
        -> bool override;

private:
    static auto _getSelf( lua_State* _l ) -> LuaRuntimeT*;
    static auto _logInfo( lua_State* _l ) -> int;
    static auto _logWarn( lua_State* _l ) -> int;
    static auto _logError( lua_State* _l ) -> int;

    auto _injectEngineApi( addonRecordT& _addon ) -> bool;
    auto _callGlobalFn( std::string_view _fnName ) -> bool;

private:
    lua_State* _l{ nullptr };
};

class WasmRuntimeT final : public addonRecordT::runtimeT {
public:
    auto load( addonRecordT& _addon ) -> bool override;
    void unload( addonRecordT& _addon ) override;
    auto callEvent( addonRecordT& _addon, std::string_view _event )
        -> bool override;
};

} // namespace mod
