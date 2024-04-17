#ifndef __IATHOOK_H
#define __IATHOOK_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>

#ifdef __cplusplus
namespace Iat_hook {
#endif

void** find_iat_func( const char* function,
                      HMODULE hModule,
                      const char* chModule,
                      const DWORD ordinal ) {
    if ( !hModule ) {
        hModule = GetModuleHandle( nullptr );
    }

    const DWORD_PTR instance = reinterpret_cast< DWORD_PTR >( hModule );
    const PIMAGE_NT_HEADERS ntHeader = reinterpret_cast< PIMAGE_NT_HEADERS >(
        instance +
        reinterpret_cast< PIMAGE_DOS_HEADER >( instance )->e_lfanew );
    PIMAGE_IMPORT_DESCRIPTOR pImports =
        reinterpret_cast< PIMAGE_IMPORT_DESCRIPTOR >(
            instance + ntHeader->OptionalHeader
                           .DataDirectory[ IMAGE_DIRECTORY_ENTRY_IMPORT ]
                           .VirtualAddress );

    try {
        while ( pImports->Name != 0 ) {
            auto mod_name = reinterpret_cast< const char* >(
                ( size_t* )( pImports->Name + ( size_t )hModule ) );

            std::string l_temp1( reinterpret_cast< const char* >( instance + pImports->Name ) );

            std::transform(l_temp1.begin(), l_temp1.end(), l_temp1.begin(),
                [](unsigned char c){ return std::tolower(c); });

            std::string l_temp2( mod_name );

            std::transform(l_temp2.begin(), l_temp2.end(), l_temp2.begin(),
                [](unsigned char c){ return std::tolower(c); });

            if ( strcmp( l_temp1.data(),
                         l_temp2.data() ) == 0 ) {
            MessageBoxA( 0, "1\n", "test", 0 );

                if ( pImports->OriginalFirstThunk != 0 ) {
            MessageBoxA( 0, "2\n", "test", 0 );
                    const PIMAGE_THUNK_DATA pThunk =
                        reinterpret_cast< PIMAGE_THUNK_DATA >(
                            instance + pImports->OriginalFirstThunk );
                    MessageBoxA( 0, function, "test", 0 );

                    for ( ptrdiff_t j = 0; pThunk[ j ].u1.AddressOfData != 0;
                          j++ ) {
                        if ( strcmp(
                                 reinterpret_cast< PIMAGE_IMPORT_BY_NAME >(
                                     instance + pThunk[ j ].u1.AddressOfData )
                                     ->Name,
                                 function ) == 0 ) {
            MessageBoxA( 0, "3\n", "test", 0 );
                            void** pAddress =
                                reinterpret_cast< void** >(
                                    instance + pImports->FirstThunk ) +
                                j;

                            pImports++;

                            return ( pAddress );
                        }
                    }

                } else {
            MessageBoxA( 0, "4\n", "test", 0 );
                    void** pFunctions = reinterpret_cast< void** >(
                        instance + pImports->FirstThunk );

                    for ( ptrdiff_t j = 0; pFunctions[ j ] != nullptr; j++ ) {
                        if ( pFunctions[ j ] ==
                             GetProcAddress( GetModuleHandle( mod_name ),
                                             function ) ) {
            MessageBoxA( 0, "5\n", "test", 0 );
                            pImports++;

                            return ( &pFunctions[ j ] );
                        }
                    }
                }
            }

            pImports++;
        }

    } catch ( const std::exception& _exception ) {
            MessageBoxA( 0, "6\n", "test", 0 );
    }

    try {
        for ( IMAGE_IMPORT_DESCRIPTOR* iid = pImports; iid->Name != 0; iid++ ) {
            if ( chModule != NULL ) {
            MessageBoxA( 0, "7\n", "test", 0 );
                char* mod_name =
                    ( char* )( ( size_t* )( iid->Name + ( size_t )hModule ) );

                if ( lstrcmpiA( chModule, mod_name ) ) {
            MessageBoxA( 0, "7\n", "test", 1 );
                    continue;
                }
            }

            for ( int func_idx = 0;
                  *( func_idx + ( void** )( iid->FirstThunk +
                                            ( size_t )hModule ) ) != NULL;
                  func_idx++ ) {
                size_t mod_func_ptr_ord = ( size_t )( *(
                    func_idx + ( size_t* )( iid->OriginalFirstThunk +
                                            ( size_t )hModule ) ) );

                if ( IMAGE_SNAP_BY_ORDINAL( mod_func_ptr_ord ) ) {
            MessageBoxA( 0, "8\n", "test", 1 );
                    if ( chModule != NULL && ordinal != 0 &&
            MessageBoxA( 0, "9\n", "test", 1 );
                         ( ordinal == IMAGE_ORDINAL( mod_func_ptr_ord ) ) ) {
                        return ( func_idx + ( void** )( iid->FirstThunk +
                                                        ( size_t )hModule ) );
                    }

                } else if ( function != NULL && function[ 0 ] != 0 ) {
            MessageBoxA( 0, "10\n", "test", 1 );
                    char* mod_func_name =
                        ( char* )( mod_func_ptr_ord + ( size_t )hModule + 2 );
                    const intptr_t nmod_func_name = ( intptr_t )mod_func_name;

                    if ( nmod_func_name >= 0 &&
                         !lstrcmpA( function, mod_func_name ) ) {
            MessageBoxA( 0, "11\n", "test", 1 );
                        return ( func_idx + ( void** )( iid->FirstThunk +
                                                        ( size_t )hModule ) );
                    }
                }
            }
        }

    } catch ( const std::exception& _exception ) {
            MessageBoxA( 0, "12\n", "test", 1 );
    }

    return ( 0 );
}

uintptr_t detour_iat_ptr( const char* function,
                          void* newfunction,
                          HMODULE hModule = NULL,
                          const char* chModule = NULL,
                          const DWORD ordinal = 0 ) {
    void** func_ptr = find_iat_func( function, hModule, chModule, ordinal );

    if ( ( !func_ptr ) || ( *func_ptr == newfunction ) ||
         ( *func_ptr == NULL ) ) {
        return ( 0 );
    }

    DWORD old_rights, new_rights = PAGE_READWRITE;

    VirtualProtect( func_ptr, sizeof( uintptr_t ), new_rights, &old_rights );

    uintptr_t ret = ( uintptr_t )*func_ptr;
    *func_ptr = newfunction;

    VirtualProtect( func_ptr, sizeof( uintptr_t ), old_rights, &new_rights );

    return ( ret );
}

#ifdef __cplusplus
};
#endif

#endif //__IATHOOK_H
