#ifndef __HELPERS_H
#define __HELPERS_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>

typedef BOOL( WINAPI* LPFN_ISTOPLEVELWINDOW )( HWND );
LPFN_ISTOPLEVELWINDOW fnIsTopLevelWindow = NULL;

BOOL _fnIsTopLevelWindow( HWND hWnd ) {
    /*  IsTopLevelWindow is not available on all versions of Windows.
     *  Use GetModuleHandle to get a handle to the DLL that contains the
     * function and GetProcAddress to get a pointer to the function if
     * available.
     */
    if ( fnIsTopLevelWindow == NULL ) {
        fnIsTopLevelWindow = ( LPFN_ISTOPLEVELWINDOW )GetProcAddress(
            GetModuleHandleA( "user32" ), "IsTopLevelWindow" );

        if ( fnIsTopLevelWindow == NULL ) {
            fnIsTopLevelWindow = ( LPFN_ISTOPLEVELWINDOW )-1;
        }
    }

    if ( fnIsTopLevelWindow != ( LPFN_ISTOPLEVELWINDOW )-1 ) {
        return ( fnIsTopLevelWindow( hWnd ) );
    }
    /* If no avail, use older method which is available in Win2000+ */
    return ( GetAncestor( hWnd, GA_ROOT ) == hWnd );
}

BOOL IsValueIntAtom( DWORD dw ) {
    return ( ( HIWORD( dw ) == 0 ) && ( LOWORD( dw ) < 0xC000 ) );
}

BOOL IsSystemClassNameA( LPCSTR _classNameA ) {
    const std::array< LPCSTR, 38 > l_systemClassNamesA = {
        "BUTTON",          "COMBOBOX",           "EDIT",
        "LISTBOX",         "MDICLIENT",          "RICHEDIT",
        "RICHEDIT_CLASS",  "SCROLLBAR",          "STATIC",
        "ANIMATE_CLASS",   "DATETIMEPICK_CLASS", "HOTKEY_CLASS",
        "LINK_CLASS",      "MONTHCAL_CLASS",     "NATIVEFNTCTL_CLASS",
        "PROGRESS_CLASS",  "REBARCLASSNAME",     "STANDARD_CLASSES",
        "STATUSCLASSNAME", "TOOLBARCLASSNAME",   "TOOLTIPS_CLASS",
        "TRACKBAR_CLASS",  "UPDOWN_CLASS",       "WC_BUTTON",
        "WC_COMBOBOX",     "WC_COMBOBOXEX",      "WC_EDIT",
        "WC_HEADER",       "WC_LISTBOX",         "WC_IPADDRESS",
        "WC_LINK",         "WC_LISTVIEW",        "WC_NATIVEFONTCTL",
        "WC_PAGESCROLLER", "WC_SCROLLBAR",       "WC_STATIC",
        "WC_TABCONTROL",   "WC_TREEVIEW" };

    for ( const LPCSTR _systemClassNameA : l_systemClassNamesA ) {
        if ( !lstrcmpiA( _classNameA, _systemClassNameA ) ) {
            return ( true );
        }
    }

    return ( false );
}

BOOL IsSystemClassNameW( LPCWSTR _classNameW ) {
    const std::array< LPCWSTR, 38 > l_systemClassNamesA = {
        L"BUTTON",          L"COMBOBOX",           L"EDIT",
        L"LISTBOX",         L"MDICLIENT",          L"RICHEDIT",
        L"RICHEDIT_CLASS",  L"SCROLLBAR",          L"STATIC",
        L"ANIMATE_CLASS",   L"DATETIMEPICK_CLASS", L"HOTKEY_CLASS",
        L"LINK_CLASS",      L"MONTHCAL_CLASS",     L"NATIVEFNTCTL_CLASS",
        L"PROGRESS_CLASS",  L"REBARCLASSNAME",     L"STANDARD_CLASSES",
        L"STATUSCLASSNAME", L"TOOLBARCLASSNAME",   L"TOOLTIPS_CLASS",
        L"TRACKBAR_CLASS",  L"UPDOWN_CLASS",       L"WC_BUTTON",
        L"WC_COMBOBOX",     L"WC_COMBOBOXEX",      L"WC_EDIT",
        L"WC_HEADER",       L"WC_LISTBOX",         L"WC_IPADDRESS",
        L"WC_LINK",         L"WC_LISTVIEW",        L"WC_NATIVEFONTCTL",
        L"WC_PAGESCROLLER", L"WC_SCROLLBAR",       L"WC_STATIC",
        L"WC_TABCONTROL",   L"WC_TREEVIEW" };

    for ( const LPCWSTR _systemClassNameA : l_systemClassNamesA ) {
        if ( !lstrcmpiW( _classNameW, _systemClassNameA ) ) {
            return ( true );
        }
    }

    return ( false );
}

#endif //__HELPERS_H
