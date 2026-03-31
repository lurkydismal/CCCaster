#include "processSuspend.hpp"

#include <tlhelp32.h>

#include <vector>

#include "logg.hpp"

processSuspendGuard::processSuspendGuard() {
    _suspendedThreadHandles.reserve( 64 );

    const DWORD l_processId = GetCurrentProcessId();
    const DWORD l_currentThreadId = GetCurrentThreadId();

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

    do {
        if ( l_entry.th32OwnerProcessID != l_processId ) {
            continue;
        }

        if ( l_entry.th32ThreadID == l_currentThreadId ) {
            continue;
        }

        HANDLE l_thread = OpenThread(
            THREAD_SUSPEND_RESUME | THREAD_QUERY_LIMITED_INFORMATION, FALSE,
            l_entry.th32ThreadID );

        if ( l_thread == nullptr ) {
            continue;
        }

        const DWORD l_previousCount = SuspendThread( l_thread );

        if ( l_previousCount == static_cast< DWORD >( -1 ) ) {
            CloseHandle( l_thread );
            continue;
        }

        if ( l_previousCount == 0 ) {
            _suspendedThreadHandles.push_back( l_thread );
        } else {
            ResumeThread( l_thread );
            CloseHandle( l_thread );
        }
    } while ( Thread32Next( l_snapshot, &l_entry ) );

    CloseHandle( l_snapshot );
}

processSuspendGuard::~processSuspendGuard() {
    for ( HANDLE l_thread : _suspendedThreadHandles ) {
        ResumeThread( l_thread );
        CloseHandle( l_thread );
    }
}
