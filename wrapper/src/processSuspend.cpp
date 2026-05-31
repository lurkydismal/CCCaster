#include "processSuspend.hpp"

#include <tlhelp32.h>

#include <vector>

#include "logg.hpp"

processSuspendGuard::processSuspendGuard() {
    // ! FIX: This function does not work as intended and crashes the app.
    return;

    _suspendedThreadHandles.reserve( 64 );

    const DWORD l_processId = GetCurrentProcessId();
    const DWORD l_currentThreadId = GetCurrentThreadId();

    logg::trace(
        "processSuspendGuard: begin suspend scan for process {}, current "
        "thread {}",
        l_processId, l_currentThreadId );

    const HANDLE l_snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );

    if ( l_snapshot == INVALID_HANDLE_VALUE ) {
        logg::error(
            "processSuspendGuard: failed to enumerate process threads" );
        return;
    }

    THREADENTRY32 l_entry{};
    l_entry.dwSize = sizeof( l_entry );

    if ( !Thread32First( l_snapshot, &l_entry ) ) {
        logg::error( "processSuspendGuard: Thread32First failed" );
        CloseHandle( l_snapshot );
        return;
    }

    std::size_t l_seenThreads = 0;
    std::size_t l_skippedOtherProcess = 0;
    std::size_t l_skippedCurrentThread = 0;
    std::size_t l_openThreadFailures = 0;
    std::size_t l_suspendFailures = 0;
    std::size_t l_resumedNonZeroCount = 0;

    do {
        ++l_seenThreads;

        if ( l_entry.th32OwnerProcessID != l_processId ) {
            ++l_skippedOtherProcess;
            continue;
        }

        if ( l_entry.th32ThreadID == l_currentThreadId ) {
            ++l_skippedCurrentThread;
            continue;
        }

        logg::trace( "processSuspendGuard: suspending thread {}",
                     l_entry.th32ThreadID );

        HANDLE l_thread = OpenThread(
            THREAD_SUSPEND_RESUME | THREAD_QUERY_LIMITED_INFORMATION, FALSE,
            l_entry.th32ThreadID );

        if ( l_thread == nullptr ) {
            ++l_openThreadFailures;
            logg::warning(
                "processSuspendGuard: OpenThread failed for thread {}",
                l_entry.th32ThreadID );
            continue;
        }

        const DWORD l_previousCount = SuspendThread( l_thread );

        if ( l_previousCount == static_cast< DWORD >( -1 ) ) {
            ++l_suspendFailures;
            logg::warning(
                "processSuspendGuard: SuspendThread failed for thread {}",
                l_entry.th32ThreadID );
            CloseHandle( l_thread );
            continue;
        }

        if ( l_previousCount == 0 ) {
            _suspendedThreadHandles.push_back( l_thread );
            logg::debug( "processSuspendGuard: thread {} suspended",
                         l_entry.th32ThreadID );
        } else {
            ++l_resumedNonZeroCount;
            logg::debug(
                "processSuspendGuard: thread {} already suspended (count {}), "
                "not tracking handle",
                l_entry.th32ThreadID, l_previousCount );
            ResumeThread( l_thread );
            CloseHandle( l_thread );
        }
    } while ( Thread32Next( l_snapshot, &l_entry ) );

    CloseHandle( l_snapshot );

    logg::info(
        "processSuspendGuard: scan complete, seen {}, suspended {}, skipped "
        "other process {}, skipped current thread {}, open failures {}, "
        "suspend failures {}, resumed non-zero {}",
        l_seenThreads, _suspendedThreadHandles.size(), l_skippedOtherProcess,
        l_skippedCurrentThread, l_openThreadFailures, l_suspendFailures,
        l_resumedNonZeroCount );
}

processSuspendGuard::~processSuspendGuard() {
    logg::trace( "processSuspendGuard: restoring {} suspended threads",
                 _suspendedThreadHandles.size() );

    for ( HANDLE l_thread : _suspendedThreadHandles ) {
        if ( ResumeThread( l_thread ) == static_cast< DWORD >( -1 ) ) {
            logg::warning( "processSuspendGuard: ResumeThread failed" );
        }

        CloseHandle( l_thread );
    }

    logg::debug( "processSuspendGuard: restore complete" );
}
