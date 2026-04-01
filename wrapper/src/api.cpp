#include "api.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <tlhelp32.h>

#include <atomic>
#include <bit>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <span>

#include "logg.hpp"
#include "memoryLock.hpp"
#include "processSuspend.hpp"
#include "storage.hpp"

namespace {

storage_t g_patches;
std::once_flag g_noPatchesFlag;
std::atomic< bool > g_noPatches{ false };

auto resolveAddress( uintptr_t _address ) -> uintptr_t {
    const auto l_moduleBase =
        std::bit_cast< uintptr_t >( GetModuleHandle( nullptr ) );
    logg::trace( "resolveAddress input={} moduleBase={}", _address, l_moduleBase );

    if ( l_moduleBase == _address ) {
        logg::warning( "resolveAddress received module base itself; refusing write/read" );
        return ( 0 );
    }

    const uintptr_t l_resolved =
        ( _address > l_moduleBase ) ? _address : ( l_moduleBase + _address );
    logg::trace( "resolveAddress resolved={}", l_resolved );
    return ( l_resolved );
}

auto isRangeAccessible( uintptr_t _address, size_t _bytesAmount, bool _write )
    -> bool {
    uintptr_t l_current = _address;
    const uintptr_t l_end = _address + _bytesAmount;

    while ( l_current < l_end ) {
        MEMORY_BASIC_INFORMATION l_memoryInfo{};

        if ( VirtualQuery( reinterpret_cast< LPCVOID >( l_current ),
                           &l_memoryInfo, sizeof( l_memoryInfo ) ) == 0 ) {
            logg::trace( "isRangeAccessible VirtualQuery failed at {}", l_current );
            return ( false );
        }

        if ( l_memoryInfo.State != MEM_COMMIT ) {
            logg::trace( "isRangeAccessible non-committed page at {}", l_current );
            return ( false );
        }

        const DWORD l_protect = l_memoryInfo.Protect & 0xFFu;

        if ( ( l_protect == PAGE_NOACCESS ) || ( l_protect == PAGE_GUARD ) ) {
            logg::trace( "isRangeAccessible blocked protection={} at {}", l_protect, l_current );
            return ( false );
        }

        if ( _write ) {
            const bool l_writable = ( l_protect == PAGE_READWRITE ) ||
                                    ( l_protect == PAGE_WRITECOPY ) ||
                                    ( l_protect == PAGE_EXECUTE_READWRITE ) ||
                                    ( l_protect == PAGE_EXECUTE_WRITECOPY );

            if ( !l_writable ) {
                logg::trace( "isRangeAccessible region is read-only protection={} at {}", l_protect,
                             l_current );
                return ( false );
            }
        }

        const auto l_regionBase =
            std::bit_cast< uintptr_t >( l_memoryInfo.BaseAddress );
        const uintptr_t l_regionEnd = l_regionBase + l_memoryInfo.RegionSize;

        if ( l_regionEnd <= l_current ) {
            logg::trace( "isRangeAccessible invalid regionEnd={} current={}", l_regionEnd, l_current );
            return ( false );
        }

        l_current = l_regionEnd;
    }

    return ( true );
}

} // namespace

namespace wrapper {

auto setNoPatches( bool _value ) -> bool {
    logg::trace( "wrapper::setNoPatches old={}, new={}", g_noPatches.load(),
                 _value );

    bool l_didRun = false;

    std::call_once( g_noPatchesFlag, [ & ] -> void {
        g_noPatches = _value;

        l_didRun = true;
    } );

    logg::trace( "wrapper::setNoPatches was{} set",
                 ( l_didRun ? "" : " not " ) );

    return ( l_didRun );
}

[[nodiscard]] auto makePatch( uintptr_t _address,
                              const std::byte* _bytes,
                              size_t _bytesAmount ) -> storage_t::handle_t {
    logg::trace( "wrapper::makePatch addr={} size={}", _address, _bytesAmount );

    if ( g_noPatches ) {
        logg::warning( "wrapper::makePatch no patches is enabled" );

        return ( storage_t::g_invalidHandle );
    }

    if ( !_address || !_bytes || !_bytesAmount ) {
        logg::warning( "wrapper::makePatch invalid arguments" );

        return ( storage_t::g_invalidHandle );
    }

    const processSuspendGuard_t l_suspendGuard;

    const auto l_handle =
        g_patches.addPatch( _address, std::span{ _bytes, _bytesAmount } );

    if ( l_handle == storage_t::g_invalidHandle ) {
        logg::error( "Patch creation failed" );
    } else {
        logg::info( "wrapper::makePatch handle={}", l_handle );
    }

    return ( l_handle );
}

[[nodiscard]] auto removePatch( storage_t::handle_t _id ) -> bool {
    logg::trace( "wrapper::removePatch handle={}", _id );

    if ( g_noPatches ) {
        logg::warning( "wrapper::removePatch no patches is enabled" );

        return ( false );
    }

    const processSuspendGuard_t l_suspendGuard;

    const bool l_result = g_patches.removePatch( _id );

    if ( !l_result ) {
        logg::error( "Patch removal failed" );
    } else {
        logg::info( "wrapper::removePatch removed handle={}", _id );
    }

    return ( l_result );
}

[[nodiscard]] auto readMemory( uintptr_t _address,
                               std::byte* _outBytes,
                               size_t _bytesAmount ) -> bool {
    logg::trace( "wrapper::readMemory addr={} size={}", _address,
                 _bytesAmount );

    if ( !_address || !_outBytes || !_bytesAmount ) {
        logg::warning( "wrapper::readMemory invalid arguments" );
        return ( false );
    }

    const uintptr_t l_address = resolveAddress( _address );

    if ( !l_address ) {
        logg::warning( "wrapper::readMemory address resolution failed" );
        return ( false );
    }

    if ( !isRangeAccessible( l_address, _bytesAmount, false ) ) {
        logg::warning( "wrapper::readMemory unreadable range addr={} size={}",
                       l_address, _bytesAmount );
        return ( false );
    }

    std::memcpy( _outBytes, reinterpret_cast< const std::byte* >( l_address ),
                 _bytesAmount );
    logg::debug( "wrapper::readMemory completed addr={} size={}", l_address, _bytesAmount );
    return ( true );
}

[[nodiscard]] auto writeMemory( uintptr_t _address,
                                const std::byte* _bytes,
                                size_t _bytesAmount,
                                bool _suspendProcess ) -> bool {
    logg::trace( "wrapper::writeMemory addr={} size={} suspend={}", _address,
                 _bytesAmount, _suspendProcess );

    if ( !_address || !_bytes || !_bytesAmount ) {
        logg::warning( "wrapper::writeMemory invalid arguments" );
        return ( false );
    }

    if ( g_noPatches ) {
        logg::warning( "wrapper::writeMemory no patches is enabled" );

        return ( false );
    }

    const uintptr_t l_address = resolveAddress( _address );

    if ( !l_address ) {
        logg::warning( "wrapper::writeMemory address resolution failed" );
        return ( false );
    }

    if ( !isRangeAccessible( l_address, _bytesAmount, true ) ) {
        logg::warning( "wrapper::writeMemory unwritable range addr={} size={}",
                       l_address, _bytesAmount );
        return ( false );
    }

    const std::optional< processSuspendGuard_t > l_suspendGuard =
        _suspendProcess ? std::make_optional< processSuspendGuard_t >()
                        : std::nullopt;

    const memoryLock_t l_lock( l_address, _bytesAmount );

    if ( !l_lock.ok() ) {
        logg::error( "wrapper::writeMemory memory lock failed" );
        return ( false );
    }

    std::memcpy( reinterpret_cast< std::byte* >( l_address ), _bytes,
                 _bytesAmount );
    logg::debug( "wrapper::writeMemory completed addr={} size={} suspend={}", l_address,
                 _bytesAmount, _suspendProcess );
    return ( true );
}

} // namespace wrapper
