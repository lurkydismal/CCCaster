#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <minhook.h>
#include <tlhelp32.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "_useCallback.hpp"
#include "d3d9.h"
#include "d3dx9.h"
#include "helpers.h"

#pragma comment( lib, "d3dx9.lib" )
#pragma comment( lib, "user32.lib" )

Direct3DShaderValidatorCreate9Proc m_pDirect3DShaderValidatorCreate9;
PSGPErrorProc m_pPSGPError;
PSGPSampleTextureProc m_pPSGPSampleTexture;
D3DPERF_BeginEventProc m_pD3DPERF_BeginEvent;
D3DPERF_EndEventProc m_pD3DPERF_EndEvent;
D3DPERF_GetStatusProc m_pD3DPERF_GetStatus;
D3DPERF_QueryRepeatFrameProc m_pD3DPERF_QueryRepeatFrame;
D3DPERF_SetMarkerProc m_pD3DPERF_SetMarker;
D3DPERF_SetOptionsProc m_pD3DPERF_SetOptions;
D3DPERF_SetRegionProc m_pD3DPERF_SetRegion;
DebugSetLevelProc m_pDebugSetLevel;
DebugSetMuteProc m_pDebugSetMute;
Direct3D9EnableMaximizedWindowedModeShimProc
    m_pDirect3D9EnableMaximizedWindowedModeShim;
Direct3DCreate9Proc m_pDirect3DCreate9;
Direct3DCreate9ExProc m_pDirect3DCreate9Ex;

HWND g_hFocusWindow = NULL;
HMODULE g_hWrapperModule = NULL;

HMODULE g_statesDll = NULL;
HMODULE g_d3d9dll = NULL;
HMODULE g_addonLoaderDll = NULL;

useCallbackFunction_t g_useCallback = NULL;

char WinDir[ MAX_PATH + 1 ];

// List of registered window classes and procedures
// WORD classAtom, ULONG_PTR WndProcPtr
std::vector< std::pair< WORD, ULONG_PTR > > WndProcList;

BOOL HookModule( HMODULE hmod );

HRESULT m_IDirect3DDevice9Ex::Present( CONST RECT* pSourceRect,
                                       CONST RECT* pDestRect,
                                       HWND hDestWindowOverride,
                                       CONST RGNDATA* pDirtyRegion ) {
    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$Present", 4, pSourceRect, pDestRect,
                      hDestWindowOverride, pDirtyRegion );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( ProxyInterface->Present( pSourceRect, pDestRect,
                                      hDestWindowOverride, pDirtyRegion ) );
}

HRESULT m_IDirect3DDevice9Ex::PresentEx( THIS_ CONST RECT* pSourceRect,
                                         CONST RECT* pDestRect,
                                         HWND hDestWindowOverride,
                                         CONST RGNDATA* pDirtyRegion,
                                         DWORD dwFlags ) {
    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$PresentEx", 5, pSourceRect, pDestRect,
                      hDestWindowOverride, pDirtyRegion, dwFlags );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( ProxyInterface->PresentEx(
        pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags ) );
}

HRESULT m_IDirect3DDevice9Ex::EndScene( void ) {
    uint16_t l_result = _useCallback( "IDirect3DDevice9Ex$EndScene" );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( ProxyInterface->EndScene() );
}

HRESULT m_IDirect3D9Ex::CreateDevice(
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    IDirect3DDevice9** ppReturnedDeviceInterface ) {
    g_hFocusWindow =
        hFocusWindow ? hFocusWindow : pPresentationParameters->hDeviceWindow;

    HRESULT hr = ProxyInterface->CreateDevice(
        Adapter, DeviceType, hFocusWindow, BehaviorFlags,
        pPresentationParameters, ppReturnedDeviceInterface );

    if ( SUCCEEDED( hr ) && ppReturnedDeviceInterface ) {
        *ppReturnedDeviceInterface = new m_IDirect3DDevice9Ex(
            ( IDirect3DDevice9Ex* )*ppReturnedDeviceInterface, this,
            IID_IDirect3DDevice9 );
    }

    uint16_t l_result = _useCallback(
        "IDirect3D9Ex$CreateDevice", 6, Adapter, DeviceType, hFocusWindow,
        BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hr );
}

HRESULT m_IDirect3DDevice9Ex::Reset(
    D3DPRESENT_PARAMETERS* pPresentationParameters ) {
    uint16_t l_result = _useCallback( "IDirect3DDevice9Ex$PreReset", 1,
                                      pPresentationParameters );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    auto hRet = ProxyInterface->Reset( pPresentationParameters );

    l_result = _useCallback( "IDirect3DDevice9Ex$PostReset", 1,
                             pPresentationParameters );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hRet );
}

HRESULT m_IDirect3D9Ex::CreateDeviceEx(
    THIS_ UINT Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode,
    IDirect3DDevice9Ex** ppReturnedDeviceInterface ) {
    g_hFocusWindow = ( hFocusWindow )
                         ? ( hFocusWindow )
                         : ( pPresentationParameters->hDeviceWindow );

    HRESULT hr = ProxyInterface->CreateDeviceEx(
        Adapter, DeviceType, hFocusWindow, BehaviorFlags,
        pPresentationParameters, pFullscreenDisplayMode,
        ppReturnedDeviceInterface );

    if ( SUCCEEDED( hr ) && ppReturnedDeviceInterface ) {
        *ppReturnedDeviceInterface = new m_IDirect3DDevice9Ex(
            *ppReturnedDeviceInterface, this, IID_IDirect3DDevice9Ex );
    }

    uint16_t l_result =
        _useCallback( "IDirect3D9Ex$CreateDeviceEx", 7, Adapter, DeviceType,
                      hFocusWindow, BehaviorFlags, pPresentationParameters,
                      pFullscreenDisplayMode, ppReturnedDeviceInterface );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hr );
}

HRESULT m_IDirect3DDevice9Ex::ResetEx(
    THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode ) {
    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$PreResetEx", 2,
                      pPresentationParameters, pFullscreenDisplayMode );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    auto hRet = ProxyInterface->ResetEx( pPresentationParameters,
                                         pFullscreenDisplayMode );

    l_result = _useCallback( "IDirect3DDevice9Ex$PostResetEx", 2,
                             pPresentationParameters, pFullscreenDisplayMode );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hRet );
}

LRESULT WINAPI
CustomWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, int idx ) {
    uint16_t l_result = _useCallback( "CustomWindowProcedure", 5, hWnd, uMsg,
                                      wParam, lParam, idx );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( true );
    }

    if ( ( hWnd == g_hFocusWindow ) ||
         // skip child windows like buttons, edit boxes, etc.
         ( _fnIsTopLevelWindow( hWnd ) ) ) {
        switch ( uMsg ) {
            case WM_ACTIVATE: {
                if ( LOWORD( wParam ) == WA_INACTIVE ) {
                    if ( ( HWND )lParam == NULL ) {
                        return ( 0 );
                    }

                    DWORD dwPID = 0;
                    GetWindowThreadProcessId( ( HWND )lParam, &dwPID );

                    if ( dwPID != GetCurrentProcessId() ) {
                        return ( 0 );
                    }
                }

                break;
            }

            case WM_NCACTIVATE: {
                if ( LOWORD( wParam ) == WA_INACTIVE ) {
                    return ( 0 );
                }

                break;
            }

            case WM_ACTIVATEAPP: {
                if ( wParam == FALSE ) {
                    return ( 0 );
                }

                break;
            }

            case WM_KILLFOCUS: {
                if ( ( HWND )wParam == NULL ) {
                    return ( 0 );
                }

                DWORD dwPID = 0;
                GetWindowThreadProcessId( ( HWND )wParam, &dwPID );

                if ( dwPID != GetCurrentProcessId() ) {
                    return ( 0 );
                }

                break;
            }
        }
    }

    WNDPROC OrigProc = WNDPROC( WndProcList[ idx ].second );

    return ( OrigProc( hWnd, uMsg, wParam, lParam ) );
}

LRESULT WINAPI CustomWndProcA( HWND hWnd,
                               UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam ) {
    uint16_t l_result =
        _useCallback( "CustomWindowProcedureA", 4, hWnd, uMsg, wParam, lParam );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( true );
    }

    WORD wClassAtom = GetClassWord( hWnd, GCW_ATOM );

    if ( wClassAtom ) {
        for ( unsigned int i = 0; i < WndProcList.size(); i++ ) {
            if ( WndProcList[ i ].first == wClassAtom ) {
                return ( CustomWndProc( hWnd, uMsg, wParam, lParam, i ) );
            }
        }
    }

    // We should never reach here, but having safeguards anyway is good
    return ( DefWindowProcA( hWnd, uMsg, wParam, lParam ) );
}

LRESULT WINAPI CustomWndProcW( HWND hWnd,
                               UINT uMsg,
                               WPARAM wParam,
                               LPARAM lParam ) {
    uint16_t l_result =
        _useCallback( "CustomWindowProcedureW", 4, hWnd, uMsg, wParam, lParam );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( true );
    }

    WORD wClassAtom = GetClassWord( hWnd, GCW_ATOM );

    if ( wClassAtom ) {
        for ( unsigned int i = 0; i < WndProcList.size(); i++ ) {
            if ( WndProcList[ i ].first == wClassAtom ) {
                return ( CustomWndProc( hWnd, uMsg, wParam, lParam, i ) );
            }
        }
    }

    // We should never reach here, but having safeguards anyway is good
    return ( DefWindowProcW( hWnd, uMsg, wParam, lParam ) );
}

typedef ATOM( __stdcall* RegisterClassA_fn )( const WNDCLASSA* );
typedef ATOM( __stdcall* RegisterClassW_fn )( const WNDCLASSW* );
typedef ATOM( __stdcall* RegisterClassExA_fn )( const WNDCLASSEXA* );
typedef ATOM( __stdcall* RegisterClassExW_fn )( const WNDCLASSEXW* );

RegisterClassA_fn oRegisterClassA = NULL;
RegisterClassW_fn oRegisterClassW = NULL;
RegisterClassExA_fn oRegisterClassExA = NULL;
RegisterClassExW_fn oRegisterClassExW = NULL;

ATOM __stdcall hk_RegisterClassA( WNDCLASSA* lpWndClass ) {
    // argument is a class name
    if ( !IsValueIntAtom( DWORD( lpWndClass->lpszClassName ) ) ) {
        // skip system classes like buttons,
        // common controls, etc.
        if ( IsSystemClassNameA( lpWndClass->lpszClassName ) ) {
            return ( oRegisterClassA( lpWndClass ) );
        }
    }

    ULONG_PTR pWndProc = ULONG_PTR( lpWndClass->lpfnWndProc );
    lpWndClass->lpfnWndProc = CustomWndProcA;
    WORD wClassAtom = oRegisterClassA( lpWndClass );

    if ( wClassAtom != 0 ) {
        WndProcList.emplace_back( wClassAtom, pWndProc );
    }

    return ( wClassAtom );
}

ATOM __stdcall hk_RegisterClassW( WNDCLASSW* lpWndClass ) {
    // argument is a class name
    if ( !IsValueIntAtom( DWORD( lpWndClass->lpszClassName ) ) ) {
        // skip system classes like buttons,
        // common controls, etc.
        if ( IsSystemClassNameW( lpWndClass->lpszClassName ) ) {
            return ( oRegisterClassW( lpWndClass ) );
        }
    }

    ULONG_PTR pWndProc = ULONG_PTR( lpWndClass->lpfnWndProc );
    lpWndClass->lpfnWndProc = CustomWndProcW;
    WORD wClassAtom = oRegisterClassW( lpWndClass );

    if ( wClassAtom != 0 ) {
        WndProcList.emplace_back( wClassAtom, pWndProc );
    }

    return ( wClassAtom );
}

ATOM __stdcall hk_RegisterClassExA( WNDCLASSEXA* lpWndClass ) {
    // argument is a class name
    if ( !IsValueIntAtom( DWORD( lpWndClass->lpszClassName ) ) ) {
        // skip system classes like buttons,
        // common controls, etc.
        if ( IsSystemClassNameA( lpWndClass->lpszClassName ) ) {
            return ( oRegisterClassExA( lpWndClass ) );
        }
    }

    ULONG_PTR pWndProc = ULONG_PTR( lpWndClass->lpfnWndProc );
    lpWndClass->lpfnWndProc = CustomWndProcA;
    WORD wClassAtom = oRegisterClassExA( lpWndClass );

    if ( wClassAtom != 0 ) {
        WndProcList.emplace_back( wClassAtom, pWndProc );
    }

    return ( wClassAtom );
}

ATOM __stdcall hk_RegisterClassExW( WNDCLASSEXW* lpWndClass ) {
    // argument is a class name
    if ( !IsValueIntAtom( DWORD( lpWndClass->lpszClassName ) ) ) {
        // skip system classes like buttons,
        // common controls, etc.
        if ( IsSystemClassNameW( lpWndClass->lpszClassName ) ) {
            return ( oRegisterClassExW( lpWndClass ) );
        }
    }

    ULONG_PTR pWndProc = ULONG_PTR( lpWndClass->lpfnWndProc );
    lpWndClass->lpfnWndProc = CustomWndProcW;
    WORD wClassAtom = oRegisterClassExW( lpWndClass );

    if ( wClassAtom != 0 ) {
        WndProcList.emplace_back( wClassAtom, pWndProc );
    }

    return ( wClassAtom );
}

typedef HWND( __stdcall* GetForegroundWindow_fn )( void );
GetForegroundWindow_fn oGetForegroundWindow = NULL;

HWND __stdcall hk_GetForegroundWindow( void ) {
    if ( g_hFocusWindow && IsWindow( g_hFocusWindow ) ) {
        return ( g_hFocusWindow );
    }

    return ( oGetForegroundWindow() );
}

typedef HWND( __stdcall* GetActiveWindow_fn )( void );
GetActiveWindow_fn oGetActiveWindow = NULL;

HWND __stdcall hk_GetActiveWindow( void ) {
    HWND hWndActive = oGetActiveWindow();

    if ( g_hFocusWindow && hWndActive == NULL && IsWindow( g_hFocusWindow ) ) {
        if ( GetCurrentThreadId() ==
             GetWindowThreadProcessId( g_hFocusWindow, NULL ) ) {
            return ( g_hFocusWindow );
        }
    }

    return ( hWndActive );
}

typedef HWND( __stdcall* GetFocus_fn )( void );
GetFocus_fn oGetFocus = NULL;

HWND __stdcall hk_GetFocus( void ) {
    HWND hWndFocus = oGetFocus();

    if ( g_hFocusWindow && hWndFocus == NULL && IsWindow( g_hFocusWindow ) ) {
        if ( GetCurrentThreadId() ==
             GetWindowThreadProcessId( g_hFocusWindow, NULL ) ) {
            return ( g_hFocusWindow );
        }
    }

    return ( hWndFocus );
}

typedef HMODULE( __stdcall* LoadLibraryA_fn )( LPCSTR lpLibFileName );
LoadLibraryA_fn oLoadLibraryA;

HMODULE __stdcall hk_LoadLibraryA( LPCSTR lpLibFileName ) {
    HMODULE hmod = oLoadLibraryA( lpLibFileName );

    if ( hmod ) {
        HookModule( hmod );
    }

    return ( hmod );
}

typedef HMODULE( __stdcall* LoadLibraryW_fn )( LPCWSTR lpLibFileName );
LoadLibraryW_fn oLoadLibraryW;

HMODULE __stdcall hk_LoadLibraryW( LPCWSTR lpLibFileName ) {
    HMODULE hmod = oLoadLibraryW( lpLibFileName );

    if ( hmod ) {
        HookModule( hmod );
    }

    return ( hmod );
}

typedef HMODULE( __stdcall* LoadLibraryExA_fn )( LPCSTR lpLibFileName,
                                                 HANDLE hFile,
                                                 DWORD dwFlags );
LoadLibraryExA_fn oLoadLibraryExA;

HMODULE __stdcall hk_LoadLibraryExA( LPCSTR lpLibFileName,
                                     HANDLE hFile,
                                     DWORD dwFlags ) {
    HMODULE hmod = oLoadLibraryExA( lpLibFileName, hFile, dwFlags );

    if ( ( hmod ) &&
         ( ( dwFlags &
             ( LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE |
               LOAD_LIBRARY_AS_IMAGE_RESOURCE ) ) == 0 ) ) {
        HookModule( hmod );
    }

    return ( hmod );
}

typedef HMODULE( __stdcall* LoadLibraryExW_fn )( LPCWSTR lpLibFileName,
                                                 HANDLE hFile,
                                                 DWORD dwFlags );
LoadLibraryExW_fn oLoadLibraryExW;

HMODULE __stdcall hk_LoadLibraryExW( LPCWSTR lpLibFileName,
                                     HANDLE hFile,
                                     DWORD dwFlags ) {
    HMODULE hmod = oLoadLibraryExW( lpLibFileName, hFile, dwFlags );

    if ( ( hmod ) &&
         ( ( dwFlags &
             ( LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE |
               LOAD_LIBRARY_AS_IMAGE_RESOURCE ) ) == 0 ) ) {
        HookModule( hmod );
    }

    return ( hmod );
}

typedef BOOL( __stdcall* FreeLibrary_fn )( HMODULE hLibModule );
FreeLibrary_fn oFreeLibrary;

BOOL __stdcall hk_FreeLibrary( HMODULE hLibModule ) {
    // We will stay in memory, thank you very much
    if ( hLibModule == g_hWrapperModule ) {
        return ( true );
    }

    return ( oFreeLibrary( hLibModule ) );
}

typedef FARPROC( __stdcall* GetProcAddress_fn )( HMODULE _moduleHandle,
                                                 LPCSTR _procName );
GetProcAddress_fn oGetProcAddress = NULL;

FARPROC __stdcall hk_GetProcAddress( HMODULE _moduleHandle, LPCSTR _procName ) {
    try {
#define GET_ADDRESS( _functionName )                                 \
    if ( lstrcmpA( _procName, #_functionName ) == 0 ) {              \
        if ( o##_functionName == NULL ) {                            \
            o##_functionName = ( _functionName##_fn )GetProcAddress( \
                _moduleHandle, _procName );                          \
        }                                                            \
                                                                     \
        return ( FARPROC )hk_##_functionName;                        \
    }

        GET_ADDRESS( RegisterClassA )
        GET_ADDRESS( RegisterClassW )
        GET_ADDRESS( RegisterClassExA )
        GET_ADDRESS( RegisterClassExW )
        GET_ADDRESS( GetForegroundWindow )
        GET_ADDRESS( GetActiveWindow )
        GET_ADDRESS( GetFocus )
        GET_ADDRESS( LoadLibraryA )
        GET_ADDRESS( LoadLibraryW )
        GET_ADDRESS( LoadLibraryExA )
        GET_ADDRESS( LoadLibraryExW )
        GET_ADDRESS( FreeLibrary )
        GET_ADDRESS( LoadLibraryExA )

#undef GET_ADDRESS

    } catch ( const std::exception& _exception ) {
    }

    return ( GetProcAddress( _moduleHandle, _procName ) );
}

BOOL HookModule( HMODULE _moduleHandle ) {
    BOOL l_returnValue = true;
    char l_modulePath[ MAX_PATH + 1 ];

    // Yeah, let's not hook ourselves
    if ( _moduleHandle == g_hWrapperModule ) {
        l_returnValue = false;

        goto EXIT;
    }

    if ( GetModuleFileNameA( _moduleHandle, l_modulePath, MAX_PATH ) ) {
        // Skip system modules

        std::string l_temp1( l_modulePath );

        std::transform( l_temp1.begin(), l_temp1.end(), l_temp1.begin(),
                        []( unsigned char c ) { return std::tolower( c ); } );

        std::string l_temp2( WinDir );

        std::transform( l_temp2.begin(), l_temp2.end(), l_temp2.begin(),
                        []( unsigned char c ) { return std::tolower( c ); } );

        if ( !strncmp( l_modulePath, WinDir, strlen( WinDir ) ) ) {
            l_returnValue = false;

            goto EXIT;
        }
    }

#define DETOUR_HOOK( _functionName )                                    \
    if ( o##_functionName == NULL ) {                                   \
        MH_CreateHook(                                                  \
            ( void* )GetProcAddress( _moduleHandle, #_functionName ),   \
            ( void* )hk_##_functionName, ( void** )&o##_functionName ); \
                                                                        \
    } else {                                                            \
        MH_CreateHook(                                                  \
            ( void* )GetProcAddress( _moduleHandle, #_functionName ),   \
            ( void* )hk_##_functionName, NULL );                        \
    }

    DETOUR_HOOK( RegisterClassA )
    DETOUR_HOOK( RegisterClassW )
    DETOUR_HOOK( RegisterClassExA )
    DETOUR_HOOK( RegisterClassExW )
    DETOUR_HOOK( GetForegroundWindow )
    DETOUR_HOOK( GetActiveWindow )
    DETOUR_HOOK( GetFocus )
    DETOUR_HOOK( LoadLibraryA )
    DETOUR_HOOK( LoadLibraryW )
    DETOUR_HOOK( LoadLibraryExA )
    DETOUR_HOOK( LoadLibraryExW )
    DETOUR_HOOK( FreeLibrary )
    DETOUR_HOOK( GetProcAddress )

#undef DETOUR_HOOK

EXIT:
    MH_EnableHook( MH_ALL_HOOKS );

    return ( l_returnValue );
}

static BOOL HookImportedModules( void ) {
    BOOL l_returnValue = true;

    HMODULE hModule;
    HMODULE hm;

    hModule = GetModuleHandle( 0 );

    PIMAGE_DOS_HEADER img_dos_headers = ( PIMAGE_DOS_HEADER )hModule;
    PIMAGE_NT_HEADERS img_nt_headers =
        ( PIMAGE_NT_HEADERS )( ( BYTE* )img_dos_headers +
                               img_dos_headers->e_lfanew );
    PIMAGE_IMPORT_DESCRIPTOR img_import_desc =
        ( PIMAGE_IMPORT_DESCRIPTOR )( ( BYTE* )img_dos_headers +
                                      img_nt_headers->OptionalHeader
                                          .DataDirectory
                                              [ IMAGE_DIRECTORY_ENTRY_IMPORT ]
                                          .VirtualAddress );

    if ( img_dos_headers->e_magic != IMAGE_DOS_SIGNATURE ) {
        l_returnValue = false;

        goto EXIT;
    }

    for ( IMAGE_IMPORT_DESCRIPTOR* iid = img_import_desc; iid->Name != 0;
          iid++ ) {
        char* mod_name =
            ( char* )( ( size_t* )( iid->Name + ( size_t )hModule ) );
        hm = GetModuleHandleA( mod_name );

        // ual check
        if ( hm && !( GetProcAddress( hm, "DirectInput8Create" ) != NULL &&
                      GetProcAddress( hm, "DirectSoundCreate8" ) != NULL &&
                      GetProcAddress( hm, "InternetOpenA" ) != NULL ) ) {
            HookModule( hm );
        }
    }

EXIT:
    return ( l_returnValue );
}

static BOOL loadDll( std::string _DLLName, HMODULE& _moduleHandle ) {
    BOOL l_returnValue = true;
    char path[ MAX_PATH ];

    GetSystemDirectoryA( path, MAX_PATH );
    strcat_s( path, ( "/" + _DLLName ).c_str() );

    _moduleHandle = LoadLibraryA( path );

    if ( !_moduleHandle ) {
        l_returnValue = false;
    }

    return ( l_returnValue );
}

extern "C" BOOL WINAPI DllMain( HMODULE hModule,
                                DWORD dwReason,
                                LPVOID lpReserved ) {
    switch ( dwReason ) {
        case DLL_PROCESS_ATTACH: {
            g_hWrapperModule = hModule;

            BOOL l_d3d9LoadResult = loadDll( "d3d9.dll", g_d3d9dll );

            if ( l_d3d9LoadResult ) {
                if ( g_d3d9dll ) {
#define GET_ADDRESS( _functionName ) \
    m_p##_functionName =             \
        ( _functionName##Proc )GetProcAddress( g_d3d9dll, #_functionName );

                    GET_ADDRESS( Direct3DShaderValidatorCreate9 )
                    GET_ADDRESS( PSGPError )
                    GET_ADDRESS( PSGPSampleTexture )
                    GET_ADDRESS( D3DPERF_BeginEvent )
                    GET_ADDRESS( D3DPERF_EndEvent )
                    GET_ADDRESS( D3DPERF_GetStatus )
                    GET_ADDRESS( D3DPERF_QueryRepeatFrame )
                    GET_ADDRESS( D3DPERF_SetMarker )
                    GET_ADDRESS( D3DPERF_SetOptions )
                    GET_ADDRESS( D3DPERF_SetRegion )
                    GET_ADDRESS( DebugSetLevel )
                    GET_ADDRESS( DebugSetMute )
                    GET_ADDRESS( Direct3D9EnableMaximizedWindowedModeShim )
                    GET_ADDRESS( Direct3DCreate9 )
                    GET_ADDRESS( Direct3DCreate9Ex )

#undef Transtelecom

                    GetSystemWindowsDirectoryA( WinDir, MAX_PATH );

                    if ( MH_Initialize() != MH_OK ) {
                        exit( 1 );
                    }

#define DETOUR_HOOK( _functionName )                                          \
    MH_CreateHookApi( L"user3.", #_functionName, ( void* )hk_##_functionName, \
                      ( void** )&o##_functionName );

                    DETOUR_HOOK( RegisterClassA )
                    DETOUR_HOOK( RegisterClassW )
                    DETOUR_HOOK( RegisterClassExA )
                    DETOUR_HOOK( RegisterClassExW )
                    DETOUR_HOOK( GetForegroundWindow )
                    DETOUR_HOOK( GetActiveWindow )
                    DETOUR_HOOK( GetFocus )
                    DETOUR_HOOK( LoadLibraryA )
                    DETOUR_HOOK( LoadLibraryW )
                    DETOUR_HOOK( LoadLibraryExA )
                    DETOUR_HOOK( LoadLibraryExW )
                    DETOUR_HOOK( FreeLibrary )
                    DETOUR_HOOK( GetProcAddress )

#undef DETOUR_HOOK

                    MH_CreateHook(
                        ( void* )GetProcAddress( g_d3d9dll, "GetProcAddress" ),
                        ( void* )hk_GetProcAddress, NULL );

                    if ( oGetForegroundWindow == NULL ) {
                        MH_CreateHook( ( void* )GetProcAddress(
                                           g_d3d9dll, "GetForegroundWindow" ),
                                       ( void* )hk_GetForegroundWindow,
                                       ( void** )&oGetForegroundWindow );

                    } else {
                        MH_CreateHook( ( void* )GetProcAddress(
                                           g_d3d9dll, "GetForegroundWindow" ),
                                       ( void* )hk_GetForegroundWindow, NULL );
                    }

                    HMODULE ole32 = GetModuleHandleA( "ole32.dll" );

                    if ( ole32 ) {
#define DETOUR_HOOK( _functionName )                                     \
    if ( o##_functionName == NULL ) {                                    \
        MH_CreateHook( ( void* )GetProcAddress( ole32, #_functionName ), \
                       ( void* )hk_##_functionName,                      \
                       ( void** )&o##_functionName );                    \
    } else {                                                             \
        MH_CreateHook( ( void* )GetProcAddress( ole32, #_functionName ), \
                       ( void* )hk_##_functionName, NULL );              \
    }

                        DETOUR_HOOK( RegisterClassA )
                        DETOUR_HOOK( RegisterClassW )
                        DETOUR_HOOK( RegisterClassExA )
                        DETOUR_HOOK( RegisterClassExW )
                        DETOUR_HOOK( GetActiveWindow )

#undef DETOUR_HOOK
                    }

                    MH_EnableHook( MH_ALL_HOOKS );

                    HookImportedModules();
                }

                // Wake mode for OS >= Windows Vista
                uint32_t l_executionThreadStateFlags =
                    ( ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED |
                      ES_AWAYMODE_REQUIRED );

                if ( !SetThreadExecutionState( l_executionThreadStateFlags ) ) {
                    // Wake mode for OS < Windows Vista
                    l_executionThreadStateFlags ^= ES_AWAYMODE_REQUIRED;

                    SetThreadExecutionState( l_executionThreadStateFlags );
                }

                g_addonLoaderDll = LoadLibraryA( "addon_loader.dll" );
                g_statesDll = GetModuleHandleA( "states.dll" );

                if ( !g_statesDll ) {
                    exit( 1 );
                }

                g_useCallback = ( useCallbackFunction_t )GetProcAddress(
                    g_statesDll, "useCallback" );

                if ( !g_useCallback ) {
                    exit( 1 );
                }

            } else {
                exit( 1 );
            }

            break;
        }

        case DLL_PROCESS_DETACH: {
            if ( g_d3d9dll ) {
                MH_Uninitialize();

                FreeLibrary( g_d3d9dll );
            }

            if ( g_addonLoaderDll ) {
                FreeLibrary( g_addonLoaderDll );
            }

            break;
        }
    }

    return ( true );
}

extern "C" HRESULT WINAPI Direct3DShaderValidatorCreate9( void ) {
    if ( !m_pDirect3DShaderValidatorCreate9 ) {
        return ( E_FAIL );
    }

    return ( m_pDirect3DShaderValidatorCreate9() );
}

extern "C" HRESULT WINAPI PSGPError( void ) {
    if ( !m_pPSGPError ) {
        return ( E_FAIL );
    }

    return ( m_pPSGPError() );
}

extern "C" HRESULT WINAPI PSGPSampleTexture( void ) {
    if ( !m_pPSGPSampleTexture ) {
        return ( E_FAIL );
    }

    return ( m_pPSGPSampleTexture() );
}

extern "C" int WINAPI D3DPERF_BeginEvent( D3DCOLOR col, LPCWSTR wszName ) {
    if ( !m_pD3DPERF_BeginEvent ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_BeginEvent( col, wszName ) );
}

extern "C" int WINAPI D3DPERF_EndEvent( void ) {
    if ( !m_pD3DPERF_EndEvent ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_EndEvent() );
}

extern "C" DWORD WINAPI D3DPERF_GetStatus( void ) {
    if ( !m_pD3DPERF_GetStatus ) {
        return ( 0 );
    }

    return ( m_pD3DPERF_GetStatus() );
}

extern "C" BOOL WINAPI D3DPERF_QueryRepeatFrame( void ) {
    if ( !m_pD3DPERF_QueryRepeatFrame ) {
        return ( false );
    }

    return ( m_pD3DPERF_QueryRepeatFrame() );
}

extern "C" void WINAPI D3DPERF_SetMarker( D3DCOLOR col, LPCWSTR wszName ) {
    if ( m_pD3DPERF_SetMarker ) {
        return ( m_pD3DPERF_SetMarker( col, wszName ) );
    }
}

extern "C" void WINAPI D3DPERF_SetOptions( DWORD dwOptions ) {
    if ( m_pD3DPERF_SetOptions ) {
        return ( m_pD3DPERF_SetOptions( dwOptions ) );
    }
}

extern "C" void WINAPI D3DPERF_SetRegion( D3DCOLOR col, LPCWSTR wszName ) {
    if ( m_pD3DPERF_SetRegion ) {
        return ( m_pD3DPERF_SetRegion( col, wszName ) );
    }
}

extern "C" HRESULT WINAPI DebugSetLevel( DWORD dw1 ) {
    if ( !m_pDebugSetLevel ) {
        return ( E_FAIL );
    }

    return ( m_pDebugSetLevel( dw1 ) );
}

extern "C" void WINAPI DebugSetMute( void ) {
    if ( m_pDebugSetMute ) {
        return ( m_pDebugSetMute() );
    }
}

extern "C" int WINAPI Direct3D9EnableMaximizedWindowedModeShim( BOOL mEnable ) {
    if ( !m_pDirect3D9EnableMaximizedWindowedModeShim ) {
        return ( 0 );
    }

    return ( m_pDirect3D9EnableMaximizedWindowedModeShim( mEnable ) );
}

extern "C" IDirect3D9* WINAPI Direct3DCreate9( UINT SDKVersion ) {
    if ( !m_pDirect3DCreate9 ) {
        return ( nullptr );
    }

    IDirect3D9* pD3D9 = m_pDirect3DCreate9( SDKVersion );

    if ( pD3D9 ) {
        return ( new m_IDirect3D9Ex( ( IDirect3D9Ex* )pD3D9, IID_IDirect3D9 ) );
    }

    return ( nullptr );
}

extern "C" HRESULT WINAPI Direct3DCreate9Ex( UINT SDKVersion,
                                             IDirect3D9Ex** ppD3D ) {
    if ( !m_pDirect3DCreate9Ex ) {
        return ( E_FAIL );
    }

    HRESULT hr = m_pDirect3DCreate9Ex( SDKVersion, ppD3D );

    if ( SUCCEEDED( hr ) && ppD3D ) {
        *ppD3D = new m_IDirect3D9Ex( *ppD3D, IID_IDirect3D9Ex );
    }

    return ( hr );
}
