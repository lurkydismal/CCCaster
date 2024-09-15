#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "_useCallback.h"
#include "patch_callbacks.h"
#include "patch_t.h"
#include "settings_parser.h"

#define ATTACH 1

int32_t __attribute__( ( stdcall ) ) DllMain( void* _handle,
                                              uint32_t _reason,
                                              void* _ ) {
    bool l_returnValue = true;

    // Patching
    if ( _reason == ATTACH ) {
#define INLINE_DWORD( X )                                    \
    ( unsigned char )( ( unsigned )X & 0xFF ),               \
        ( unsigned char )( ( ( unsigned )X >> 8 ) & 0xFF ),  \
        ( unsigned char )( ( ( unsigned )X >> 16 ) & 0xFF ), \
        ( unsigned char )( ( ( unsigned )X >> 24 ) & 0xFF )

        // Hook game main loop
#define MM_HOOK_CALL1_ADDR ( 0x40D032 )
#define MM_HOOK_CALL2_ADDR ( 0x40D411 )
#define CC_LOOP_START_ADDR ( 0x40D330 )
        MAKE_PATCH(
            MM_HOOK_CALL1_ADDR,
            { // call gameMainLoopCallback
              0xE8,
              INLINE_DWORD( ( ( char* )&gameMainLoopCallback ) -
                            MM_HOOK_CALL1_ADDR - 5 ),
              // jmp MM_HOOK_CALL2_ADDR
              0xE9,
              INLINE_DWORD( MM_HOOK_CALL2_ADDR - MM_HOOK_CALL1_ADDR - 10 ) } );
        MAKE_PATCH(
            MM_HOOK_CALL2_ADDR,
            { // push 01
              0x6A, 0x01,
              // push 00
              0x6A, 0x00,
              // push 00
              0x6A, 0x00,
              // jmp CC_LOOP_START_ADDR+6 ( AFTER )
              0xE9,
              INLINE_DWORD( CC_LOOP_START_ADDR - MM_HOOK_CALL2_ADDR - 5 ) } );
        MAKE_PATCH(
            CC_LOOP_START_ADDR,
            {
                // jmp MM_HOOK_CALL1_ADDR
                0xE9,
                INLINE_DWORD( MM_HOOK_CALL1_ADDR - CC_LOOP_START_ADDR - 5 ),
                // nop
                0x90
                // AFTER:
            } );

#undef MM_HOOK_CALL1_ADDR
#undef MM_HOOK_CALL2_ADDR
#undef CC_LOOP_START_ADDR

        // Copy the value of the current menu index (edi) to a location we
        // control. This hack uses the strategy of jumping around the extra
        // spaces left in between code.
#if 0
        MAKE_PATCH( 0x4294D1, {
                // mov edi,[esi+40]
                0x8B, 0x7E, 0x40,
                // mov [&g_currentMenuIndex],edi
                0x89, 0x3D,
                INLINE_DWORD(
                        &g_currentMenuIndex ),
                // jmp 0x4299D0 ( AFTER )
                0xE9, 0xF1, 0x04, 0x00, 0x00
                } );
        MAKE_PATCH( 0x429817, {
        // test ecx,ecx
                 0x85, 0xC9,
        // jmp 0x4294D1
                 0xE9, 0xB3, 0xFC, 0xFF, 0xFF
             } );
        MAKE_PATCH( 0x4299CB, {
                // jmp 0x429817
                0xE9, 0x47, 0xFE, 0xFF, 0xFF
                // AFTER:
                } );

        /// Code that allows us to selectively override menu confirms.
        /// The value of \c g_menuConfirmState is set to 1 if a menu confirm
        /// happens. A menu confirm will only go through if \c
        /// g_menuConfirmState is greater than 1.
        MAKE_PATCH( 0x428F52, {
                // push ebx,ecx,edx
                0x53, 0x51, 0x52,
                // lea ebx,[esp+30]
                0x8D, 0x5C, 0x24, 0x30,
                // mov ecx,&g_menuConfirmState
                0xB9,
                INLINE_DWORD( &g_menuConfirmState ),
                // jmp 0x428F64
                0xEB, 0x04
                } );
        MAKE_PATCH( 0x428F64, {
                // cmp dword ptr [ecx],01
                0x83, 0x39, 0x01,
                // mov edx,[ebx]
                0x8B, 0x13,
                // mov [ecx],edx
                0x89, 0x11,
                // jg 0x428FC8 ( LABEL_B )
                0x7F, 0x5B,
                // jmp 0x428FC4 ( LABEL_A )
                0xEB, 0x55
                } );
          MAKE_PATCH( 0x428F7A, {
              // pop edx,ecx,ebx
              0x5A, 0x59, 0x5B,
              // nop
              0x90,
              // jmp 0x428F8D ( RETURN )
              0xEB, 0x0D
              } );
          MAKE_PATCH( 0x428FC4, {
              // LABEL_A:
              // mov [ebx],eax
              0x89, 0x03,
              // jmp 0x428F7A
              0xEB, 0xB2,
              // LABEL_B:
              // mov [ecx],eax
              0x89, 0x01,
              // jmp 0x428F7A
              0xEB, 0xAE
              } );
          MAKE_PATCH( 0x428F82, {
              // cmp [esp],0x4299F5 ( return address of menu call )
              0x81, 0x3C, 0x24, 0xF5, 0x99, 0x42,
              0x00,
              // jne 0x428F8D (RETURN)
              0x75, 0x02,
              // jmp 0x428F52
              0xEB, 0xC5,
              // RETURN:
              // ret 0004
              0xC2, 0x04, 0x00
              } );
#endif

        // Detect round start
        MAKE_PATCH( 0x440D16, { // mov ecx,[&g_roundStartCounter]
                                0xB9, INLINE_DWORD( &g_roundStartCounter ),
                                // jmp 0x441002
                                0xE9, 0xE2, 0x02, 0x00, 0x00 } );
        MAKE_PATCH( 0x441002, { // mov esi,[ecx]
                                0x8B, 0x31,
                                // inc esi
                                0x46,
                                // mov [ecx],esi
                                0x89, 0x31,
                                // pop esi
                                0x5E,
                                // pop ecx
                                0x59,
                                // ret
                                0xC3 } );
        MAKE_PATCH( 0x440CC5, { // jmp 0x440D16
                                0xEB, 0x4F } );

        // Filter repeated SFX
        MAKE_PATCH( 0x4DD836, { // mov eax,g_SFXMute
                                0xB8, INLINE_DWORD( g_SFXMute ),
                                // jmp 0x4DD8B6
                                0xEB, 0x79 } );
        MAKE_PATCH( 0x4DD8B6, { // cmp byte ptr [eax+esi],00
                                0x80, 0x3C, 0x30, 0x00,
                                // jmp 0x4DDB73
                                0xE9, 0xB4, 0x02, 0x00, 0x00 } );
        MAKE_PATCH( 0x4DDB73, { // je 0x4DDEB3
                                0x0F, 0x84, 0x3A, 0x03, 0x00, 0x00,
                                // pop eax
                                0x58,
                                // jmp 0x4DDFA4
                                0xE9, 0x25, 0x04, 0x00, 0x00 } );
        MAKE_PATCH( 0x4DDEB3, { // mov eax,g_SFXFilter
                                0xB8, INLINE_DWORD( g_SFXFilter ),
                                // add byte ptr [eax+esi],01
                                0x80, 0x04, 0x30, 0x01,
                                // jmp 0x4DDF32
                                0xEB, 0x74 } );
        MAKE_PATCH( 0x4DDF32, { // cmp byte ptr [eax+esi],01
                                0x80, 0x3C, 0x30, 0x01,
                                // pop eax
                                0x58,
                                // ja 0x4DE223 (SKIP_SFX)
                                0x0F, 0x87, 0xE6, 0x02, 0x00, 0x00,
                                // jmp 0x4DDFA4
                                0xEB, 0x65 } );
        MAKE_PATCH( 0x4DDFA4, { // mov edi,[esi*4+0076C6F8]
                                0x8B, 0x3C, 0xB5, INLINE_DWORD( 0x76C6F8 ),
                                // jmp 0x4DE217 ( PLAY_SFX )
                                0xE9, 0x67, 0x02, 0x00, 0x00 } );
        MAKE_PATCH( 0x4DE210, {
                                  // push eax
                                  0x50,
                                  // jmp 0x4DD836
                                  0xE9, 0x20, 0xF6, 0xFF, 0xFF,
                                  // nop
                                  0x90
                                  // PLAY_SFX:
                                  // test edi,edi
                                  // je 0x4DE220
                                  // call 0x4F3A0
                                  // add ebp,01
                                  // add esi,01
                                  // SKIP_SFX:
                              } );

        // Mute specific SFX
#define DX_MUTED_VOLUME ( 0xFFFFD8F0u )

        MAKE_PATCH( 0x40EEA1, { // mov edx,[esp]
                                0x8B, 0x14, 0x24,
                                // cmp edx,CC_SFX_ARRAY_LENGTH
                                0x81, 0xFA, INLINE_DWORD( CC_SFX_ARRAY_LENGTH ),
                                // jmp 0x40F1D1
                                0xE9, 0x22, 0x03, 0x00, 0x00 } );
        MAKE_PATCH( 0x40F1D1, { // jnl 0x40F398 (AFTER)
                                0x0F, 0x8D, 0xC1, 0x01, 0x00, 0x00,
                                // jmp 0x40F392
                                0xE9, 0xB6, 0x01, 0x00, 0x00 } );
        MAKE_PATCH( 0x40F392, { // jne 0x40F462
                                0x0F, 0x85, 0xCA, 0x00, 0x00, 0x00,
                                // AFTER:
                                // mov edx,[eax+3C]
                                0x8B, 0x50, 0x3C,
                                // push ecx
                                0x51,
                                // push esi
                                0x56,
                                // jmp 0x40F3D5
                                0xEB, 0x3B } );
        MAKE_PATCH( 0x40F462, { // lea edx,[edx+g_SFXMute]
                                0x8D, 0x92, INLINE_DWORD( g_SFXMute ),
                                // jmp 0x40FAE5
                                0xE9, 0x78, 0x06, 0x00, 0x00 } );
        MAKE_PATCH( 0x40FAE5, { // cmp byte ptr [edx],00
                                0x80, 0x3A, 0x00,
                                // mov byte ptr [edx],00
                                0xC6, 0x02, 0x00,
                                // jmp 0x40FB01
                                0xEB, 0x14 } );
        MAKE_PATCH( 0x40FB01, { // je 0x40FB03 ( DONE_MUTE )
                                0x74, 0x05,
                                // mov ecx,DX_MUTED_VOLUME
                                0xB9, INLINE_DWORD( DX_MUTED_VOLUME ),
                                // DONE_MUTE:
                                // jmp 0x40F398 ( AFTER )
                                0xE9, 0x8B, 0xF8, 0xFF, 0xFF } );
        MAKE_PATCH( 0x40F3D5, { // jmp 0x40EEA1
                                0xE9, 0xC7, 0xFA, 0xFF, 0xFF } );

#undef DX_MUTED_VOLUME

        // Add extra textures callback
        MAKE_PATCH( 0x41BE38,
                    { // call extraTexturesCallBack
                      0xE8,
                      INLINE_DWORD( ( ( char* )&extraTexturesCallBack ) -
                                    0x41BE38 - 5 ),
                      0xC3 } );

        // Inserts a callback just after the color data is loaded into memory,
        // but before it is read into the sprites. The color values can be
        // effectively overridden here. This is only effective during character
        // select.
        MAKE_PATCH(
            0x489CD1,
            { // call characterSelectColorsCallback
              0xE8,
              INLINE_DWORD( ( ( char* )&characterSelectColorsCallback ) -
                            0x489CD1 - 5 ),
              // NOP NOP NOP
              0x90, 0x90, 0x90 } );

        // Inserts a callback just after the color data is loaded into memory,
        // but before it is read into the sprites. The color values can be
        // effectively overridden here. This is only effective during the
        // loading state.
        MAKE_PATCH( 0x448202,
                    { // push eax
                      0x50,
                      // call loadingColorsCallback
                      0xE8,
                      INLINE_DWORD( ( ( char* )&loadingColorsCallback ) -
                                    0x448202 - 1 - 5 ),
                      // pop eax
                      0x58,
                      // test eax,eax
                      0x85, 0xC0,
                      // jmp 0x448245
                      0xEB, 0x38 } );
        MAKE_PATCH( 0x448245, { // mov [esp+20],eax
                                0x89, 0x44, 0x24, 0x20,
                                // jmp 0x448BCF (AFTER)
                                0xE9, 0x81, 0x09, 0x00, 0x00 } );
        MAKE_PATCH( 0x448BC9, {
                                  // jmp 0x448202
                                  0xE9, 0x34, 0xF6, 0xFF, 0xFF,
                                  // nop
                                  0x90
                                  // AFTER:
                              } );

        // Hook game render function
        MAKE_PATCH( 0x432CD2, { // call extraDrawCallback
                                0xE8,
                                INLINE_DWORD( ( ( char* )&extraDrawCallback ) -
                                              0x432CD2 - 5 ),
                                // Push -1
                                0x6A, 0xFF,
                                // jmp 432D32
                                0xE9, 0x54, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x432D30, { // jmp 00424E09
                                0xEB, 0xA0 } );

        // Detect auto replay save
        //  MAKE_PATCH( 0x482D9B, {
        //      // lea ecx,[eax+000000D0]
        //      0x8D, 0x88, 0xD0, 0x00, 0x00, 0x00,
        //      // mov [&autoReplaySaveStatePtr],ecx
        //      0x89, 0x0D,
        //      INLINE_DWORD(
        //              &g_autoReplaySaveState ),
        //      // Rest of the code is unchanged, just shifted down
        //      0x59, 0x5E, 0x83, 0xC4, 0x10, 0xC2, 0x04, 0x00 } );

        // Control when to exit the game by ESC button
        MAKE_PATCH( 0x4A0070,
                    { // cmp byte ptr [&g_enableEscapeToExit],00
                      0x80, 0x3D, INLINE_DWORD( &g_enableEscapeToExit ), 0x00,
                      // mov ax,[005544F1]
                      0xA0, INLINE_DWORD( 0x5544F1 ),
                      // jne 0x4A0081 ( AFTER )
                      0x75, 0x03,
                      // xor ax,ax
                      0x66, 0x31, 0xC0,
                      // AFTER:
                      // and al,80
                      0x24, 0x80,
                      // Rest of the code is unchanged, just shifted down
                      // xor ecx,ecx
                      0x33, 0xC9,
                      // cmp al,80
                      0x3C, 0x80,
                      // set cl
                      0x0F, 0x94, 0xC1,
                      // mov eax,ecx
                      0x8B, 0xC1,
                      // ret
                      0xC3 } );

        // Fix Ryougi stage music looping
        MAKE_PATCH( 0x7695F6, { 0x35, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x7695EC, { 0xAA, 0xCC, 0x1E, 0x40 } );

        // Disable gamepad mappings
        MAKE_PATCH( 0x41F098, { 0x90, 0x90 } );
        MAKE_PATCH( 0x41F0A0, { 0x90, 0x90, 0x90 } );
        MAKE_PATCH( 0x4A024E, { 0x90, 0x90 } );
        MAKE_PATCH( 0x4A027F, { 0x90, 0x90, 0x90 } );
        MAKE_PATCH( 0x4A0291, { 0x90, 0x90, 0x90 } );
        MAKE_PATCH( 0x4A02A2, { 0x90, 0x90, 0x90 } );
        MAKE_PATCH( 0x4A02B4, { 0x90, 0x90, 0x90 } );
        MAKE_PATCH( 0x4A02E9, { 0x90, 0x90 } );
        MAKE_PATCH( 0x4A02F2, { 0x90, 0x90, 0x90 } );

        // Disable keyboard mappings
        MAKE_PATCH( 0x54D2C0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } );

        // Enable disabled stages
        MAKE_PATCH( 0x54CEBC, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CEC0, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CEC4, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CFA8, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CFAC, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CFB0, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF68, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF6C, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF70, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF74, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF78, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF7C, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF80, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF84, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF88, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF8C, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF90, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF94, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF98, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CF9C, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CFA0, { 0xFF, 0x00, 0x00, 0x00 } );
        MAKE_PATCH( 0x54CFA4, { 0xFF, 0x00, 0x00, 0x00 } );

        // Disables the code that sets the intro state to 0. This is so we can
        // manually set it during rollback
        MAKE_PATCH( 0x45C1F2, { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 } );

        // Prevent training mode music from reseting
        MAKE_PATCH( 0x472C6D, { // jmp 00472C74
                                0xEB, 0x05 } );
        MAKE_PATCH( 0x53B3C8, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                0xFF, 0xFF, 0xFF, 0xFF } );
        MAKE_PATCH( 0x40D25A, { // jmp 0040D262
                                0xEB } );

        // Skip MBAACC initial config window
        MAKE_PATCH( 0x04A1D42, { 0xEB, 0x0E } );
        MAKE_PATCH( 0x04A1D4A, { 0xEB } );

#undef INLINE_DWORD

        if ( readSettingsFromFile( SETTINGS_FILE_PATH ) != 0 ) {
            if ( readSettingsFromFile( SETTINGS_BACKUP_FILE_PATH ) == 0 ) {
                writeSettingsToFile( SETTINGS_FILE_PATH );

                _useCallback( "log$transaction$query",
                              "Reading Settings From Backup File\n" );

            } else {
                _useCallback( "log$transaction$query",
                              "Settings Backup File Not Found\n" );
            }
        }
    }

    return ( l_returnValue );
}
