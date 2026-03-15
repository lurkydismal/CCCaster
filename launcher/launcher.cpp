#include <windows.h>

#include <bit>
#include <cstdio>
#include <cstring>
#include <string>

constexpr std::string g_exeName = "MBAA.exe";
constexpr std::string g_dllName = "wrapper.dll";

auto main() -> int {
    STARTUPINFOA l_si{};
    PROCESS_INFORMATION l_pi{};

    l_si.cb = sizeof( l_si );

    const char* l_target = g_exeName.c_str();
    const char* l_dllPath = g_dllName.c_str();

    // 1. Start process suspended
    if ( !CreateProcessA( l_target, nullptr, nullptr, nullptr, FALSE,
                          CREATE_SUSPENDED, nullptr, nullptr, &l_si, &l_pi ) ) {
        printf( "CreateProcess failed\n" );
        return 1;
    }

    printf( "Process started suspended\n" );

    // 2. Allocate memory for DLL path
    LPVOID l_remoteMem =
        VirtualAllocEx( l_pi.hProcess, nullptr, strlen( l_dllPath ) + 1,
                        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );

    if ( !l_remoteMem ) {
        printf( "VirtualAllocEx failed\n" );
        VirtualFreeEx( l_pi.hProcess, l_remoteMem, 0, MEM_RELEASE );
        CloseHandle( l_pi.hThread );
        CloseHandle( l_pi.hProcess );
        return 1;
    }

    // 3. Write DLL path
    if ( !WriteProcessMemory( l_pi.hProcess, l_remoteMem, l_dllPath,
                              strlen( l_dllPath ) + 1, nullptr ) ) {
        printf( "WriteProcessMemory failed\n" );
        VirtualFreeEx( l_pi.hProcess, l_remoteMem, 0, MEM_RELEASE );
        CloseHandle( l_pi.hThread );
        CloseHandle( l_pi.hProcess );
        return 1;
    }

    // 4. Get LoadLibraryA address
    auto l_loadLibrary = std::bit_cast< LPVOID >(
        GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "LoadLibraryA" ) );

    if ( !l_loadLibrary ) {
        printf( "GetProcAddress failed\n" );
        CloseHandle( l_pi.hThread );
        CloseHandle( l_pi.hProcess );
        return 1;
    }

    // 5. Create remote thread
    HANDLE l_thread = CreateRemoteThread(
        l_pi.hProcess, nullptr, 0,
        std::bit_cast< LPTHREAD_START_ROUTINE >( l_loadLibrary ), l_remoteMem,
        0, nullptr );

    if ( !l_thread ) {
        printf( "CreateRemoteThread failed\n" );
        VirtualFreeEx( l_pi.hProcess, l_remoteMem, 0, MEM_RELEASE );
        CloseHandle( l_pi.hThread );
        CloseHandle( l_pi.hProcess );
        return 1;
    }

    WaitForSingleObject( l_thread, INFINITE );

    printf( "DLL injected\n" );

    DWORD l_exitCode{};
    if ( !GetExitCodeThread( l_thread, &l_exitCode ) ) {
        printf( "GetExitCodeThread failed\n" );
        VirtualFreeEx( l_pi.hProcess, l_remoteMem, 0, MEM_RELEASE );
        CloseHandle( l_thread );
        CloseHandle( l_pi.hThread );
        CloseHandle( l_pi.hProcess );
        return 1;
    }

    if ( !l_exitCode ) {
        printf( "LoadLibrary failed in remote process\n" );
        VirtualFreeEx( l_pi.hProcess, l_remoteMem, 0, MEM_RELEASE );
        CloseHandle( l_thread );
        CloseHandle( l_pi.hThread );
        CloseHandle( l_pi.hProcess );
        return 1;
    }

    VirtualFreeEx( l_pi.hProcess, l_remoteMem, 0, MEM_RELEASE );

    CloseHandle( l_thread );

    // 6. Resume main thread
    printf( "Resuming process\n" );
    ResumeThread( l_pi.hThread );

    CloseHandle( l_pi.hThread );
    CloseHandle( l_pi.hProcess );

    return 0;
}
