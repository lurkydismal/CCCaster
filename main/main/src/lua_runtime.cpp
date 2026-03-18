#include "lua_runtime.hpp"

#include <filesystem>

#include "logg.hpp"

namespace mod {

static constexpr std::string g_runtimeKey = "mod.lua.runtime";

LuaRuntimeT::~LuaRuntimeT() {
    if ( _l != nullptr ) {
        lua_close( _l );

        _l = nullptr;
    }
}

auto LuaRuntimeT::_getSelf( lua_State* _l ) -> LuaRuntimeT* {
    lua_getfield( _l, LUA_REGISTRYINDEX, g_runtimeKey.c_str() );

    auto* l_ptr = static_cast< LuaRuntimeT* >( lua_touserdata( _l, -1 ) );

    lua_pop( _l, 1 );

    return ( l_ptr );
}

auto LuaRuntimeT::_logInfo( lua_State* _l ) -> int {
    [[maybe_unused]] auto* l_self = _getSelf( _l );

    const char* l_text = lua_tostring( _l, 1 );

    logg::info( "[mod]: {}", ( l_text ? l_text : "" ) );

    return ( 0 );
}

auto LuaRuntimeT::_logWarn( lua_State* _l ) -> int {
    const char* l_text = lua_tostring( _l, 1 );

    logg::info( "[mod] warn: {}", ( l_text ? l_text : "" ) );

    return ( 0 );
}

auto LuaRuntimeT::_logError( lua_State* _l ) -> int {
    const char* l_text = lua_tostring( _l, 1 );

    logg$error( "[mod] error: {}", ( l_text ? l_text : "" ) );

    return ( 0 );
}

auto LuaRuntimeT::_injectEngineApi( addonRecordT& _addon ) -> bool {
    lua_newtable( _l );

    lua_pushcfunction( _l, &LuaRuntimeT::_logInfo );
    lua_setfield( _l, -2, "log_info" );

    lua_pushcfunction( _l, &LuaRuntimeT::_logWarn );
    lua_setfield( _l, -2, "log_warn" );

    lua_pushcfunction( _l, &LuaRuntimeT::_logError );
    lua_setfield( _l, -2, "log_error" );

    lua_pushinteger(
        _l, static_cast< lua_Integer >( _addon.manifest.api_version ) );
    lua_setfield( _l, -2, "api_version" );

    lua_pushstring( _l, _addon.id.c_str() );
    lua_setfield( _l, -2, "id" );

    lua_setglobal( _l, "Engine" );

    lua_newtable( _l );
    lua_pushlightuserdata( _l, this );
    lua_setfield( _l, -2, g_runtimeKey.c_str() );
    lua_setfield( _l, LUA_REGISTRYINDEX, g_runtimeKey.c_str() );

    return ( true );
}

auto LuaRuntimeT::_callGlobalFn( std::string_view _fnName ) -> bool {
    lua_getglobal( _l, std::string( _fnName ).c_str() );

    if ( !lua_isfunction( _l, -1 ) ) {
        lua_pop( _l, 1 );

        return ( true );
    }

    if ( lua_pcall( _l, 0, 0, 0 ) != LUA_OK ) {
        const char* l_err = lua_tostring( _l, -1 );

        logg$error( "[lua] {}", ( l_err ? l_err : "unknown error" ) );

        lua_pop( _l, 1 );

        return ( false );
    }

    return ( true );
}

auto LuaRuntimeT::load( addonRecordT& _addon ) -> bool {
    if ( _l != nullptr ) {
        return ( false );
    }

    _l = luaL_newstate();

    if ( _l == nullptr ) {
        return ( false );
    }

    luaL_openlibs( _l );

    if ( !_injectEngineApi( _addon ) ) {
        return ( false );
    }

    const auto l_entryPath =
        ( _addon.root / _addon.manifest.entry ).lexically_normal();

    if ( luaL_dofile( _l, l_entryPath.string().c_str() ) != LUA_OK ) {
        const char* l_err = lua_tostring( _l, -1 );

        logg$error( "[lua] load failed for {}: {}", _addon.id,
                    ( l_err ? l_err : "unknown error" ) );

        lua_pop( _l, 1 );

        return ( false );
    }

    return ( _callGlobalFn( "on_load" ) );
}

void LuaRuntimeT::unload( [[maybe_unused]] addonRecordT& _addon ) {
    if ( _l == nullptr ) {
        return;
    }

    ( void )_callGlobalFn( "on_unload" );

    lua_close( _l );

    _l = nullptr;
}

auto LuaRuntimeT::callEvent( [[maybe_unused]] addonRecordT& _addon,
                             std::string_view _event ) -> bool {
    return ( _callGlobalFn( _event ) );
}

auto WasmRuntimeT::load( addonRecordT& _addon ) -> bool {
    logg$error( "[wasm] runtime not implemented for addon: {}", _addon.id );

    return ( false );
}

void WasmRuntimeT::unload( [[maybe_unused]] addonRecordT& _addon ) {}

auto WasmRuntimeT::callEvent( [[maybe_unused]] addonRecordT& _addon,
                              [[maybe_unused]] std::string_view _event )
    -> bool {
    return ( false );
}

} // namespace mod
