#include "api.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <tlhelp32.h>

#include <atomic>
#include <bit>
#include <climits>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <span>
#include <vector>
#include <unordered_map>

#include "logg.hpp"
#include "memoryLock.hpp"
#include "processSuspend.hpp"
#include "storage.hpp"

namespace {

storage_t g_patches;
std::once_flag g_noPatchesFlag;
std::atomic< bool > g_noPatches{ false };
std::mutex g_detoursMutex;
using detourRecord_t = struct detourRecord {
    uintptr_t targetAddress;
    uintptr_t detourAddress;
    uintptr_t trampolineAddress;
    size_t overwriteLength;
    std::vector< std::byte > originalBytes;
};
std::unordered_map< storage_t::handle_t, detourRecord_t > g_detours;
std::atomic< storage_t::handle_t > g_nextDetourHandle{ 1u };
constexpr size_t g_detourPatchLength = 10u; // push imm32 + call rel32
constexpr size_t g_trampolineJumpLength = 5u;

void printBytes( std::span< const std::byte > _bytes ) {
    std::string l_buffer = "[";

    for ( auto l_byte : _bytes ) {
        l_buffer += std::format( "{:X} ", static_cast< uint8_t >( l_byte ) );
    }

    l_buffer += "]";

    logg::trace( "[BYTES] {}", l_buffer );
}

void printBytes( uintptr_t _address, size_t _length ) {
    printBytes(
        std::span{ std::bit_cast< const std::byte* >( _address ), _length } );
}

auto writeRelativeCall( std::byte* _buffer,
                        uintptr_t _instructionAddress,
                        uintptr_t _destination ) -> bool {
    constexpr uint8_t l_callOpcode = 0xE8u;
    if ( !_buffer ) {
        return ( false );
    }

    const int64_t l_delta = static_cast< int64_t >( _destination ) -
                            static_cast< int64_t >( _instructionAddress + 5u );
    if ( ( l_delta < INT32_MIN ) || ( l_delta > INT32_MAX ) ) {
        return ( false );
    }

    const int32_t l_rel32 = static_cast< int32_t >( l_delta );
    _buffer[ 0 ] = static_cast< std::byte >( l_callOpcode );
    std::memcpy( _buffer + 1, &l_rel32, sizeof( l_rel32 ) );
    return ( true );
}

auto writeRelativeJump( std::byte* _buffer,
                        uintptr_t _instructionAddress,
                        uintptr_t _destination ) -> bool {
    constexpr uint8_t l_jmpOpcode = 0xE9u;
    if ( !_buffer ) {
        return ( false );
    }

    const int64_t l_delta = static_cast< int64_t >( _destination ) -
                            static_cast< int64_t >( _instructionAddress + 5u );
    if ( ( l_delta < INT32_MIN ) || ( l_delta > INT32_MAX ) ) {
        return ( false );
    }

    const int32_t l_rel32 = static_cast< int32_t >( l_delta );
    _buffer[ 0 ] = static_cast< std::byte >( l_jmpOpcode );
    std::memcpy( _buffer + 1, &l_rel32, sizeof( l_rel32 ) );
    return ( true );
}

auto resolveAddress( uintptr_t _address ) -> uintptr_t {
    const auto l_moduleBase =
        std::bit_cast< uintptr_t >( GetModuleHandle( nullptr ) );
    logg::trace( "resolveAddress input=0x{:X} moduleBase=0x{:X}", _address,
                 l_moduleBase );

    if ( l_moduleBase == _address ) {
        logg::warning(
            "resolveAddress received module base itself; refusing write/read" );
        return ( 0 );
    }

    const uintptr_t l_resolved =
        ( _address > l_moduleBase ) ? _address : ( l_moduleBase + _address );
    logg::trace( "resolveAddress resolved=0x{:X}", l_resolved );
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
            logg::trace( "isRangeAccessible VirtualQuery failed at {}",
                         l_current );
            return ( false );
        }

        if ( l_memoryInfo.State != MEM_COMMIT ) {
            logg::trace( "isRangeAccessible non-committed page at {}",
                         l_current );
            return ( false );
        }

        const DWORD l_protect = l_memoryInfo.Protect & 0xFFu;

        if ( ( l_protect == PAGE_NOACCESS ) || ( l_protect == PAGE_GUARD ) ) {
            logg::trace( "isRangeAccessible blocked protection={} at {}",
                         l_protect, l_current );
            return ( false );
        }

        if ( _write ) {
            const bool l_writable = ( l_protect == PAGE_READWRITE ) ||
                                    ( l_protect == PAGE_WRITECOPY ) ||
                                    ( l_protect == PAGE_EXECUTE_READWRITE ) ||
                                    ( l_protect == PAGE_EXECUTE_WRITECOPY );

            if ( !l_writable ) {
                logg::trace(
                    "isRangeAccessible region is read-only protection={} at {}",
                    l_protect, l_current );
                return ( false );
            }
        }

        const auto l_regionBase =
            std::bit_cast< uintptr_t >( l_memoryInfo.BaseAddress );
        const uintptr_t l_regionEnd = l_regionBase + l_memoryInfo.RegionSize;

        if ( l_regionEnd <= l_current ) {
            logg::trace( "isRangeAccessible invalid regionEnd={} current={}",
                         l_regionEnd, l_current );
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
                              size_t _bytesAmount,
                              bool _suspendProcess ) -> storage_t::handle_t {
    logg::trace( "wrapper::makePatch addr={} size={} suspend={}", _address,
                 _bytesAmount, _suspendProcess );

    if ( g_noPatches ) {
        logg::warning( "wrapper::makePatch no patches is enabled" );

        return ( storage_t::g_invalidHandle );
    }

    if ( !_address || !_bytes || !_bytesAmount ) {
        logg::warning( "wrapper::makePatch invalid arguments" );

        return ( storage_t::g_invalidHandle );
    }

    const std::optional< processSuspendGuard_t > l_suspendGuard =
        _suspendProcess ? std::make_optional< processSuspendGuard_t >()
                        : std::nullopt;

    const auto l_handle =
        g_patches.addPatch( _address, std::span{ _bytes, _bytesAmount } );

    if ( l_handle == storage_t::g_invalidHandle ) {
        logg::error( "Patch creation failed" );
    } else {
        logg::info( "wrapper::makePatch handle={}", l_handle );
    }

    return ( l_handle );
}

[[nodiscard]] auto removePatch( storage_t::handle_t _id, bool _suspendProcess )
    -> bool {
    logg::trace( "wrapper::removePatch handle={} suspend={}", _id,
                 _suspendProcess );

    if ( g_noPatches ) {
        logg::warning( "wrapper::removePatch no patches is enabled" );

        return ( false );
    }

    const std::optional< processSuspendGuard_t > l_suspendGuard =
        _suspendProcess ? std::make_optional< processSuspendGuard_t >()
                        : std::nullopt;

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
    logg::debug( "wrapper::readMemory completed addr={} size={}", l_address,
                 _bytesAmount );
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
    logg::debug( "wrapper::writeMemory completed addr={} size={} suspend={}",
                 l_address, _bytesAmount, _suspendProcess );
    return ( true );
}

[[nodiscard]] auto createDetour( uintptr_t _targetAddress,
                                 uintptr_t _detourAddress,
                                 uintptr_t* _outTrampolineAddress,
                                 bool _suspendProcess ) -> storage_t::handle_t {
    logg::trace(
        "wrapper::createDetour begin target=0x{:X} detour=0x{:X} "
        "outTrampoline={} suspend={}",
        _targetAddress, _detourAddress,
        static_cast< const void* >( _outTrampolineAddress ), _suspendProcess );

    if ( !_targetAddress || !_detourAddress || !_outTrampolineAddress ) {
        logg::warning( "wrapper::createDetour invalid arguments" );
        return ( storage_t::g_invalidHandle );
    }

    if ( g_noPatches ) {
        logg::warning( "wrapper::createDetour patches disabled" );
        return ( storage_t::g_invalidHandle );
    }

    const uintptr_t l_targetAddress = resolveAddress( _targetAddress );
    const uintptr_t l_detourAddress = resolveAddress( _detourAddress );

    logg::debug( "wrapper::createDetour resolved target=0x{:X} detour=0x{:X}",
                 l_targetAddress, l_detourAddress );

    if ( !l_targetAddress || !l_detourAddress ) {
        logg::warning(
            "wrapper::createDetour address resolution failed target=0x{:X} "
            "detour=0x{:X}",
            l_targetAddress, l_detourAddress );
        return ( storage_t::g_invalidHandle );
    }

    constexpr size_t l_dumpLength = 32u;

    logg::info( "wrapper::createDetour bytes before patch target=0x{:X}",
                l_targetAddress );
    printBytes( l_targetAddress, l_dumpLength );

    const std::optional< processSuspendGuard_t > l_suspendGuard =
        _suspendProcess ? std::make_optional< processSuspendGuard_t >()
                        : std::nullopt;

    if ( _suspendProcess ) {
        logg::info( "wrapper::createDetour process suspended" );
    } else {
        logg::trace( "wrapper::createDetour process suspension skipped" );
    }

    std::lock_guard< std::mutex > l_lock( g_detoursMutex );
    const storage_t::handle_t l_handle = g_nextDetourHandle.fetch_add( 1u );

    logg::trace( "wrapper::createDetour handle={} allocating hook", l_handle );

    std::vector< std::byte > l_originalBytes( g_detourPatchLength );
    if ( !readMemory( l_targetAddress, l_originalBytes.data(),
                      l_originalBytes.size() ) ) {
        logg::error( "wrapper::createDetour failed to snapshot target bytes" );
        return ( storage_t::g_invalidHandle );
    }

    const size_t l_trampolineLength = g_detourPatchLength + g_trampolineJumpLength;
    auto* l_trampoline = static_cast< std::byte* >(
        VirtualAlloc( nullptr, l_trampolineLength, MEM_COMMIT | MEM_RESERVE,
                      PAGE_EXECUTE_READWRITE ) );
    if ( !l_trampoline ) {
        logg::error(
            "wrapper::createDetour failed to allocate trampoline: gle={}",
            GetLastError() );
        return ( storage_t::g_invalidHandle );
    }

    std::memcpy( l_trampoline, l_originalBytes.data(), l_originalBytes.size() );
    if ( !writeRelativeJump( l_trampoline + g_detourPatchLength,
                             std::bit_cast< uintptr_t >(
                                 l_trampoline + g_detourPatchLength ),
                             l_targetAddress + g_detourPatchLength ) ) {
        logg::error(
            "wrapper::createDetour failed to write trampoline jump: handle={}",
            l_handle );
        VirtualFree( l_trampoline, 0, MEM_RELEASE );
        return ( storage_t::g_invalidHandle );
    }

    std::vector< std::byte > l_patchBytes( g_detourPatchLength );
    l_patchBytes[ 0 ] = static_cast< std::byte >( 0x68u );
    const uint32_t l_callSiteImmediate = static_cast< uint32_t >( l_targetAddress );
    std::memcpy( l_patchBytes.data() + 1, &l_callSiteImmediate, sizeof( uint32_t ) );
    if ( !writeRelativeCall( l_patchBytes.data() + 5, l_targetAddress + 5,
                             l_detourAddress ) ) {
        logg::error(
            "wrapper::createDetour failed to encode call for handle={}",
            l_handle );
        VirtualFree( l_trampoline, 0, MEM_RELEASE );
        return ( storage_t::g_invalidHandle );
    }

    {
        const memoryLock_t l_memoryLock( l_targetAddress, g_detourPatchLength );
        if ( !l_memoryLock.ok() ) {
            logg::error(
                "wrapper::createDetour failed to lock target memory for handle={}",
                l_handle );
            VirtualFree( l_trampoline, 0, MEM_RELEASE );
            return ( storage_t::g_invalidHandle );
        }
        std::memcpy( std::bit_cast< std::byte* >( l_targetAddress ),
                     l_patchBytes.data(), l_patchBytes.size() );
    }

    *_outTrampolineAddress = std::bit_cast< uintptr_t >( l_trampoline );

    logg::info( "wrapper::createDetour bytes after patch target=0x{:X}",
                l_targetAddress );
    printBytes( l_targetAddress, l_dumpLength );

    logg::info( "wrapper::createDetour trampoline bytes address=0x{:X}",
                *_outTrampolineAddress );
    printBytes( *_outTrampolineAddress, l_dumpLength );

    g_detours.emplace( l_handle, detourRecord_t{ .targetAddress = l_targetAddress,
                                                 .detourAddress = l_detourAddress,
                                                 .trampolineAddress =
                                                     *_outTrampolineAddress,
                                                 .overwriteLength =
                                                     g_detourPatchLength,
                                                 .originalBytes =
                                                     std::move( l_originalBytes ) } );

    logg::info(
        "wrapper::createDetour success handle={} target=0x{:X} detour=0x{:X} "
        "trampoline=0x{:X}",
        l_handle, l_targetAddress, l_detourAddress, *_outTrampolineAddress );

    return ( l_handle );
}

[[nodiscard]] auto removeDetour( storage_t::handle_t _id ) -> bool {
    std::lock_guard< std::mutex > l_lock( g_detoursMutex );
    const auto l_found = g_detours.find( _id );
    if ( l_found == g_detours.end() ) {
        return ( false );
    }

    const detourRecord_t l_record = l_found->second;
    {
        const memoryLock_t l_memoryLock( l_record.targetAddress,
                                         l_record.overwriteLength );
        if ( !l_memoryLock.ok() ) {
            return ( false );
        }
        std::memcpy( std::bit_cast< std::byte* >( l_record.targetAddress ),
                     l_record.originalBytes.data(),
                     l_record.originalBytes.size() );
    }

    if ( l_record.trampolineAddress ) {
        if ( !VirtualFree( std::bit_cast< LPVOID >( l_record.trampolineAddress ),
                           0, MEM_RELEASE ) ) {
            logg::warning( "wrapper::removeDetour failed to release trampoline 0x{:X} (gle={})",
                           l_record.trampolineAddress, GetLastError() );
        }
    }

    g_detours.erase( l_found );
    return ( true );
}

} // namespace wrapper
