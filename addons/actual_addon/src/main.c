#include <stdbool.h>
#include <stdint.h>

#include "patch_callbacks.h"
#include "patch_t.h"

#define ATTACH 1

int32_t __attribute__( ( stdcall ) ) DllMain( void* _handle,
        uint32_t _reason,
        void* _ ) {
    bool l_returnValue = true;

    // Patching
    if ( _reason == ATTACH ) {
#define INLINE_DWORD( X )                                               \
        ( unsigned char )( ( unsigned )X & 0xFF ),               \
        ( unsigned char )( ( ( unsigned )X >> 8 ) & 0xFF ),  \
        ( unsigned char )( ( ( unsigned )X >> 16 ) & 0xFF ), \
        ( unsigned char )( ( ( unsigned )X >> 24 ) & 0xFF )

        // Hook game main loop
        {
#define MM_HOOK_CALL1_ADDR ( 0x40D032 )
#define MM_HOOK_CALL2_ADDR ( 0x40D411 )
#define CC_LOOP_START_ADDR ( 0x40D330 )
            {
                uint8_t l_gameMainLoopCallbackPatch[] = {
                    // call gameMainLoopCallback
                    0xE8,
                    INLINE_DWORD( ( ( char* )&gameMainLoopCallback ) -
                            MM_HOOK_CALL1_ADDR - 5 ),
                    // jmp MM_HOOK_CALL2_ADDR
                    0xE9,
                    INLINE_DWORD( MM_HOOK_CALL2_ADDR - MM_HOOK_CALL1_ADDR - 10 ) };

                static patch_t l_gameMainLoopCallbackPatch1;
                l_gameMainLoopCallbackPatch1 = patchCreate( MM_HOOK_CALL1_ADDR, l_gameMainLoopCallbackPatch,
                        sizeof( l_gameMainLoopCallbackPatch ) );
            }

            {
                uint8_t l_gameMainLoopCallbackPatch[] = {
                    // push 01
                    0x6A, 0x01,
                    // push 00
                    0x6A, 0x00,
                    // push 00
                    0x6A, 0x00,
                    // jmp CC_LOOP_START_ADDR+6 ( AFTER )
                    0xE9,
                    INLINE_DWORD( CC_LOOP_START_ADDR - MM_HOOK_CALL2_ADDR - 5 ) };

                static patch_t l_gameMainLoopCallbackPatch2;
                l_gameMainLoopCallbackPatch2 =
                    patchCreate( MM_HOOK_CALL2_ADDR, l_gameMainLoopCallbackPatch,
                            sizeof( l_gameMainLoopCallbackPatch ) );
            }

            {
                uint8_t l_gameMainLoopCallbackPatch[] = {
                    // jmp MM_HOOK_CALL1_ADDR
                    0xE9,
                    INLINE_DWORD( MM_HOOK_CALL1_ADDR - CC_LOOP_START_ADDR - 5 ),
                    // nop
                    0x90
                        // AFTER:
                };

                static patch_t l_gameMainLoopCallbackPatch3;
                l_gameMainLoopCallbackPatch3 =
                    patchCreate( CC_LOOP_START_ADDR, l_gameMainLoopCallbackPatch,
                            sizeof( l_gameMainLoopCallbackPatch ) );
            }

#undef MM_HOOK_CALL1_ADDR
#undef MM_HOOK_CALL2_ADDR
#undef CC_LOOP_START_ADDR
        }

        // Copy the value of the current menu index (edi) to a location we
        // control. This hack uses the strategy of jumping around the extra
        // spaces left in between code.
        // {
        // static patch_t l_currentMenuIndexPatch1 = patchCreate(
        //     0x4294D1,
        //     {
        // mov edi,[esi+40]
        //         0x8B, 0x7E, 0x40,
        // mov [&g_currentMenuIndex],edi
        //         0x89, 0x3D,
        //         INLINE_DWORD(
        //             &g_currentMenuIndex ),
        // jmp 0x4299D0 ( AFTER )
        //         0xE9, 0xF1, 0x04, 0x00, 0x00
        //     } );

        // static patch_t l_currentMenuIndexPatch2 = patchCreate(
        //     0x429817,
        //     {
        // test ecx,ecx
        //         0x85, 0xC9,
        // jmp 0x4294D1
        //         0xE9, 0xB3, 0xFC, 0xFF, 0xFF
        //     } );

        // static patch_t l_currentMenuIndexPatch3 = patchCreate(
        //     0x4299CB,
        //     {
        // jmp 0x429817
        //         0xE9, 0x47, 0xFE, 0xFF, 0xFF
        //                                      // AFTER:
        //     } );
        // }

        /// Code that allows us to selectively override menu confirms.
        /// The value of \c g_menuConfirmState is set to 1 if a menu confirm
        /// happens. A menu confirm will only go through if \c g_menuConfirmState is
        /// greater than 1.
        /// {
        // static patch_t l_menuConfirmStatePatch1 = patchCreate(
        //     0x428F52,
        //     {
        // push ebx,ecx,edx
        //         0x53, 0x51, 0x52,
        // lea ebx,[esp+30]
        //         0x8D, 0x5C, 0x24, 0x30,
        // mov ecx,&g_menuConfirmState
        //         0xB9,
        //         INLINE_DWORD( &g_menuConfirmState ),
        // jmp 0x428F64
        // 0xEB, 0x04
        //     } );

        // static patch_t l_menuConfirmStatePatch2 = patchCreate(
        //     0x428F64,
        //     {
        // cmp dword ptr [ecx],01
        //         0x83, 0x39, 0x01,
        // mov edx,[ebx]
        //         0x8B, 0x13,
        // mov [ecx],edx
        //         0x89, 0x11,
        // jg 0x428FC8 ( LABEL_B )
        //         0x7F, 0x5B,
        // jmp 0x428FC4 ( LABEL_A )
        //         0xEB, 0x55
        //     } );

        // static patch_t l_menuConfirmStatePatch3 = patchCreate(
        //     0x428F7A,
        //     {
        // pop edx,ecx,ebx
        //         0x5A, 0x59, 0x5B,
        // nop
        //         0x90,
        // jmp 0x428F8D ( RETURN )
        //         0xEB, 0x0D
        //     } );

        // static patch_t l_menuConfirmStatePatch4 = patchCreate( 0x428FC4,
        //                                          {
        //                                              // LABEL_A:
        // mov [ebx],eax
        //                                              0x89, 0x03,
        // jmp 0x428F7A
        //                                              0xEB, 0xB2,
        //                                                          // LABEL_B:
        // mov [ecx],eax
        //                                              0x89, 0x01,
        // jmp 0x428F7A
        //                                              0xEB, 0xAE
        //                                          } );

        // static patch_t l_menuConfirmStatePatch5 = patchCreate(
        //     0x428F82,
        //     {
        // cmp [esp],0x4299F5 ( return address of menu call )
        //         0x81, 0x3C, 0x24, 0xF5, 0x99, 0x42,
        //         0x00,
        // jne 0x428F8D (RETURN)
        //         0x75, 0x02,
        // jmp 0x428F52
        //         0xEB, 0xC5,
        //                          // RETURN:
        // ret 0004
        //         0xC2, 0x04, 0x00
        //     } );
        // }

        // Detect round start
        {
            {
                uint8_t l_roundStartCounterPatch[] = {
                    // mov ecx,[&g_roundStartCounter]
                    0xB9, INLINE_DWORD( &g_roundStartCounter ),
                    // jmp 0x441002
                    0xE9, 0xE2, 0x02, 0x00, 0x00 };

                static patch_t l_roundStartCounterPatch1;
                l_roundStartCounterPatch1 =
                    patchCreate( 0x440D16, l_roundStartCounterPatch,
                            sizeof( l_roundStartCounterPatch ) );
            }

            {
                uint8_t l_roundStartCounterPatch[] = { // mov esi,[ecx]
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
                    0xC3 };

                static patch_t l_roundStartCounterPatch2;
                l_roundStartCounterPatch2 =
                    patchCreate( 0x441002, l_roundStartCounterPatch,
                            sizeof( l_roundStartCounterPatch ) );
            }

            {
                uint8_t l_roundStartCounterPatch[] = { // jmp 0x440D16
                    0xEB, 0x4F };

                static patch_t l_roundStartCounterPatch3;
                l_roundStartCounterPatch3 =
                    patchCreate( 0x440CC5, l_roundStartCounterPatch,
                            sizeof( l_roundStartCounterPatch ) );
            }
        }

        // Filter repeated SFX
        {
            {
                uint8_t l_SFXMuteFilterPatch[] = { // mov eax,g_SFXMute
                    0xB8, INLINE_DWORD( g_SFXMute ),
                    // jmp 0x4DD8B6
                    0xEB, 0x79 };

                static patch_t l_SFXMuteFilterPatch1;
                l_SFXMuteFilterPatch1 =
                    patchCreate( 0x4DD836, l_SFXMuteFilterPatch,
                            sizeof( l_SFXMuteFilterPatch ) );
            }

            {
                uint8_t l_SFXMuteFilterPatch[] = { // cmp byte ptr [eax+esi],00
                    0x80, 0x3C, 0x30, 0x00,
                    // jmp 0x4DDB73
                    0xE9, 0xB4, 0x02, 0x00, 0x00 };

                static patch_t l_SFXMuteFilterPatch2;
                l_SFXMuteFilterPatch2 =
                    patchCreate( 0x4DD8B6, l_SFXMuteFilterPatch,
                            sizeof( l_SFXMuteFilterPatch ) );
            }

            {
                uint8_t l_SFXMuteFilterPatch[] = {
                    // je 0x4DDEB3
                    0x0F, 0x84, 0x3A, 0x03, 0x00, 0x00,
                    // pop eax
                    0x58,
                    // jmp 0x4DDFA4
                    0xE9, 0x25, 0x04, 0x00, 0x00 };

                static patch_t l_SFXMuteFilterPatch3;
                l_SFXMuteFilterPatch3 =
                    patchCreate( 0x4DDB73, l_SFXMuteFilterPatch,
                            sizeof( l_SFXMuteFilterPatch ) );
            }

            {
                uint8_t l_SFXMuteFilterPatch[] = {
                    // mov eax,g_SFXFilter
                    0xB8, INLINE_DWORD( g_SFXFilter ),
                    // add byte ptr [eax+esi],01
                    0x80, 0x04, 0x30, 0x01,
                    // jmp 0x4DDF32
                    0xEB, 0x74 };

                static patch_t l_SFXMuteFilterPatch4;
                l_SFXMuteFilterPatch4 =
                    patchCreate( 0x4DDEB3, l_SFXMuteFilterPatch,
                            sizeof( l_SFXMuteFilterPatch ) );
            }

            {
                uint8_t l_SFXMuteFilterPatch[] = { // cmp byte ptr [eax+esi],01
                    0x80, 0x3C, 0x30, 0x01,
                    // pop eax
                    0x58,
                    // ja 0x4DE223 (SKIP_SFX)
                    0x0F, 0x87, 0xE6, 0x02, 0x00,
                    0x00,
                    // jmp 0x4DDFA4
                    0xEB, 0x65 };

                static patch_t l_SFXMuteFilterPatch5;
                l_SFXMuteFilterPatch5 =
                    patchCreate( 0x4DDF32, l_SFXMuteFilterPatch,
                            sizeof( l_SFXMuteFilterPatch ) );
            }

            {
                uint8_t l_SFXMuteFilterPatch[] = {
                    // mov edi,[esi*4+0076C6F8]
                    0x8B, 0x3C, 0xB5, INLINE_DWORD( 0x76C6F8 ),
                    // jmp 0x4DE217 ( PLAY_SFX )
                    0xE9, 0x67, 0x02, 0x00, 0x00 };

                static patch_t l_SFXMuteFilterPatch6;
                l_SFXMuteFilterPatch6 =
                    patchCreate( 0x4DDFA4, l_SFXMuteFilterPatch,
                            sizeof( l_SFXMuteFilterPatch ) );
            }

            {
                uint8_t l_SFXMuteFilterPatch[] = {
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
                };

                static patch_t l_SFXMuteFilterPatch7;
                l_SFXMuteFilterPatch7 =
                    patchCreate( 0x4DE210, l_SFXMuteFilterPatch,
                            sizeof( l_SFXMuteFilterPatch ) );
            }
        }

        // Mute specific SFX
        {
#define DX_MUTED_VOLUME ( 0xFFFFD8F0u )

            {
                uint8_t l_SFXMutePatch[] = { // mov edx,[esp]
                    0x8B, 0x14, 0x24,
                    // cmp edx,CC_SFX_ARRAY_LENGTH
                    0x81, 0xFA,
                    INLINE_DWORD( CC_SFX_ARRAY_LENGTH ),
                    // jmp 0x40F1D1
                    0xE9, 0x22, 0x03, 0x00, 0x00 };

                static patch_t l_SFXMutePatch1;
                l_SFXMutePatch1 = patchCreate(
                        0x40EEA1, l_SFXMutePatch, sizeof( l_SFXMutePatch ) );
            }

            {
                uint8_t l_SFXMutePatch[] = { // jnl 0x40F398 (AFTER)
                    0x0F, 0x8D, 0xC1, 0x01, 0x00, 0x00,
                    // jmp 0x40F392
                    0xE9, 0xB6, 0x01, 0x00, 0x00 };

                static patch_t l_SFXMutePatch2;
                l_SFXMutePatch2 = patchCreate(
                        0x40F1D1, l_SFXMutePatch, sizeof( l_SFXMutePatch ) );
            }

            {
                uint8_t l_SFXMutePatch[] = { // jne 0x40F462
                    0x0F, 0x85, 0xCA, 0x00, 0x00, 0x00,
                    // AFTER:
                    // mov edx,[eax+3C]
                    0x8B, 0x50, 0x3C,
                    // push ecx
                    0x51,
                    // push esi
                    0x56,
                    // jmp 0x40F3D5
                    0xEB, 0x3B };

                static patch_t l_SFXMutePatch3;
                l_SFXMutePatch3 = patchCreate(
                        0x40F392, l_SFXMutePatch, sizeof( l_SFXMutePatch ) );
            }

            {
                uint8_t l_SFXMutePatch[] = { // lea edx,[edx+g_SFXMute]
                    0x8D, 0x92, INLINE_DWORD( g_SFXMute ),
                    // jmp 0x40FAE5
                    0xE9, 0x78, 0x06, 0x00, 0x00 };

                static patch_t l_SFXMutePatch4;
                l_SFXMutePatch4 = patchCreate(
                        0x40F462, l_SFXMutePatch, sizeof( l_SFXMutePatch ) );
            }

            {
                uint8_t l_SFXMutePatch[] = { // cmp byte ptr [edx],00
                    0x80, 0x3A, 0x00,
                    // mov byte ptr [edx],00
                    0xC6, 0x02, 0x00,
                    // jmp 0x40FB01
                    0xEB, 0x14 };

                static patch_t l_SFXMutePatch5;
                l_SFXMutePatch5 = patchCreate(
                        0x40FAE5, l_SFXMutePatch, sizeof( l_SFXMutePatch ) );
            }

            {
                uint8_t l_SFXMutePatch[] = { // je 0x40FB03 ( DONE_MUTE )
                    0x74, 0x05,
                    // mov ecx,DX_MUTED_VOLUME
                    0xB9, INLINE_DWORD( DX_MUTED_VOLUME ),
                    // DONE_MUTE:
                    // jmp 0x40F398 ( AFTER )
                    0xE9, 0x8B, 0xF8, 0xFF, 0xFF };

                static patch_t l_SFXMutePatch6;
                l_SFXMutePatch6 = patchCreate(
                        0x40FB01, l_SFXMutePatch, sizeof( l_SFXMutePatch ) );
            }

            {
                uint8_t l_SFXMutePatch[] = { // jmp 0x40EEA1
                    0xE9, 0xC7, 0xFA, 0xFF, 0xFF };

                static patch_t l_SFXMutePatch7;
                l_SFXMutePatch7 = patchCreate(
                        0x40F3D5, l_SFXMutePatch, sizeof( l_SFXMutePatch ) );
            }

#undef DX_MUTED_VOLUME
        }

        // Add extra textures callback
        {
            {
                uint8_t l_extraTexturesCallBackPatch[] = {
                    // call extraTexturesCallBack
                    0xE8,
                    INLINE_DWORD( ( ( char* )&extraTexturesCallBack ) - 0x41BE38 -
                            5 ),
                    0xC3 };

                static patch_t l_extraTexturesCallBackPatch1;
                l_extraTexturesCallBackPatch1 =
                    patchCreate( 0x41BE38, l_extraTexturesCallBackPatch,
                            sizeof( l_extraTexturesCallBackPatch ) );
            }
        }

        // Inserts a callback just after the color data is loaded into memory, but
        // before it is read into the sprites. The color values can be effectively
        // overridden here. This is only effective during character select.
        {
            {
                uint8_t l_characterSelectColorsCallbackPatch[] = {
                    // call characterSelectColorsCallback
                    0xE8,
                    INLINE_DWORD( ( ( char* )&characterSelectColorsCallback ) -
                            0x489CD1 - 5 ),
                    // NOP NOP NOP
                    0x90, 0x90, 0x90 };

                static patch_t l_characterSelectColorsCallbackPatch1;
                l_characterSelectColorsCallbackPatch1 =
                    patchCreate( 0x489CD1, l_characterSelectColorsCallbackPatch,
                            sizeof( l_characterSelectColorsCallbackPatch ) );
            }
        }

        // Inserts a callback just after the color data is loaded into memory, but
        // before it is read into the sprites. The color values can be effectively
        // overridden here. This is only effective during the loading state.
        {
            {
                uint8_t l_loadingColorsCallbackPatch[] = {
                    // push eax
                    0x50,
                    // call loadingColorsCallback
                    0xE8,
                    INLINE_DWORD( ( ( char* )&loadingColorsCallback ) - 0x448202 -
                            1 - 5 ),
                    // pop eax
                    0x58,
                    // test eax,eax
                    0x85, 0xC0,
                    // jmp 0x448245
                    0xEB, 0x38 };

                static patch_t l_loadingColorsCallbackPatch1;
                l_loadingColorsCallbackPatch1 =
                    patchCreate( 0x448202, l_loadingColorsCallbackPatch,
                            sizeof( l_loadingColorsCallbackPatch ) );
            }

            {
                uint8_t l_loadingColorsCallbackPatch[] = { // mov [esp+20],eax
                    0x89, 0x44, 0x24, 0x20,
                    // jmp 0x448BCF (AFTER)
                    0xE9, 0x81, 0x09, 0x00,
                    0x00 };

                static patch_t l_loadingColorsCallbackPatch2;
                l_loadingColorsCallbackPatch2 =
                    patchCreate( 0x448245, l_loadingColorsCallbackPatch,
                            sizeof( l_loadingColorsCallbackPatch ) );
            }

            // Write this last due to dependencies
            {
                uint8_t l_loadingColorsCallbackPatch[] = {
                    // jmp 0x448202
                    0xE9, 0x34, 0xF6, 0xFF, 0xFF,
                    // nop
                    0x90
                        // AFTER:
                };

                static patch_t l_loadingColorsCallbackPatch3;
                l_loadingColorsCallbackPatch3 =
                    patchCreate( 0x448BC9, l_loadingColorsCallbackPatch,
                            sizeof( l_loadingColorsCallbackPatch ) );
            }
        }

        {
            {
                uint8_t l_extraDrawCallbackPatch[] = {
                    // call extraDrawCallback
                    0xE8,
                    INLINE_DWORD( ( ( char* )&extraDrawCallback ) - 0x432CD2 - 5 ),
                    // Push -1
                    0x6A, 0xFF,
                    // jmp 432D32
                    0xE9, 0x54, 0x00, 0x00, 0x00 };

                static patch_t l_extraDrawCallbackPatch1;
                l_extraDrawCallbackPatch1 =
                    patchCreate( 0x432CD2, l_extraDrawCallbackPatch,
                            sizeof( l_extraDrawCallbackPatch ) );
            }

            {
                uint8_t l_extraDrawCallbackPatch[] = { // jmp 00424E09
                    0xEB, 0xA0 };

                static patch_t l_extraDrawCallbackPatch2;
                l_extraDrawCallbackPatch2 =
                    patchCreate( 0x432D30, l_extraDrawCallbackPatch,
                            sizeof( l_extraDrawCallbackPatch ) );
            }
        }

        // Detect auto replay save
        // {
        // static patch_t l_autoReplaySaveStatePatch1 = patchCreate(
        //     0x482D9B,
        //     {
        // lea ecx,[eax+000000D0]
        // 0x8D, 0x88, 0xD0, 0x00, 0x00, 0x00,
        // mov [&autoReplaySaveStatePtr],ecx
        //       0x89, 0x0D,
        //       INLINE_DWORD(
        //           &g_autoReplaySaveState ),
        //       // Rest of the code is unchanged, just shifted down
        //       0x59, 0x5E, 0x83, 0xC4, 0x10, 0xC2, 0x04, 0x00 } );
        // }

        // Control when to exit the game by ESC button
        {
            {
                uint8_t l_enableEscapeToExitPatch[] = {
                    // cmp byte ptr [&g_enableEscapeToExit],00
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
                    0xC3 };

                static patch_t l_enableEscapeToExitPatch1;
                l_enableEscapeToExitPatch1 =
                    patchCreate( 0x4A0070, l_enableEscapeToExitPatch,
                            sizeof( l_enableEscapeToExitPatch ) );
            }
        }

        // Fix Ryougi stage music looping
        {
            {
                uint8_t l_ryougiStageMusicLoopingPatch[] = { 0x35, 0x00, 0x00,
                    0x00 };

                static patch_t l_ryougiStageMusicLoopingPatch1;
                l_ryougiStageMusicLoopingPatch1 =
                    patchCreate( 0x7695F6, l_ryougiStageMusicLoopingPatch,
                            sizeof( l_ryougiStageMusicLoopingPatch ) );
            }

            {
                uint8_t l_ryougiStageMusicLoopingPatch[] = { 0xAA, 0xCC, 0x1E,
                    0x40 };

                static patch_t l_ryougiStageMusicLoopingPatch2;
                l_ryougiStageMusicLoopingPatch2 =
                    patchCreate( 0x7695EC, l_ryougiStageMusicLoopingPatch,
                            sizeof( l_ryougiStageMusicLoopingPatch ) );
            }
        }

        // Disable gamepad mappings
        {
            {
                uint8_t l_gamepadMappingsPatch[] = { 0x90, 0x90 };

                static patch_t l_gamepadMappingsPatch1;
                l_gamepadMappingsPatch1 =
                    patchCreate( 0x41F098, l_gamepadMappingsPatch,
                            sizeof( l_gamepadMappingsPatch ) );
            }

            {
                uint8_t l_gamepadMappingsPatch[] = { 0x90, 0x90, 0x90 };

                static patch_t l_gamepadMappingsPatch2;
                l_gamepadMappingsPatch2 =
                    patchCreate( 0x41F0A0, l_gamepadMappingsPatch,
                            sizeof( l_gamepadMappingsPatch ) );
            }

            {
                uint8_t l_gamepadMappingsPatch[] = { 0x90, 0x90 };

                static patch_t l_gamepadMappingsPatch3;
                l_gamepadMappingsPatch3 =
                    patchCreate( 0x4A024E, l_gamepadMappingsPatch,
                            sizeof( l_gamepadMappingsPatch ) );
            }

            {
                uint8_t l_gamepadMappingsPatch[] = { 0x90, 0x90, 0x90 };

                static patch_t l_gamepadMappingsPatch4;
                l_gamepadMappingsPatch4 =
                    patchCreate( 0x4A027F, l_gamepadMappingsPatch,
                            sizeof( l_gamepadMappingsPatch ) );
            }

            {
                uint8_t l_gamepadMappingsPatch[] = { 0x90, 0x90, 0x90 };

                static patch_t l_gamepadMappingsPatch5;
                l_gamepadMappingsPatch5 =
                    patchCreate( 0x4A0291, l_gamepadMappingsPatch,
                            sizeof( l_gamepadMappingsPatch ) );
            }

            {
                uint8_t l_gamepadMappingsPatch[] = { 0x90, 0x90, 0x90 };

                static patch_t l_gamepadMappingsPatch6;
                l_gamepadMappingsPatch6 =
                    patchCreate( 0x4A02A2, l_gamepadMappingsPatch,
                            sizeof( l_gamepadMappingsPatch ) );
            }

            {
                uint8_t l_gamepadMappingsPatch[] = { 0x90, 0x90, 0x90 };

                static patch_t l_gamepadMappingsPatch7;
                l_gamepadMappingsPatch7 =
                    patchCreate( 0x4A02B4, l_gamepadMappingsPatch,
                            sizeof( l_gamepadMappingsPatch ) );
            }

            {
                uint8_t l_gamepadMappingsPatch[] = { 0x90, 0x90 };

                static patch_t l_gamepadMappingsPatch8;
                l_gamepadMappingsPatch8 =
                    patchCreate( 0x4A02E9, l_gamepadMappingsPatch,
                            sizeof( l_gamepadMappingsPatch ) );
            }

            {
                uint8_t l_gamepadMappingsPatch[] = { 0x90, 0x90, 0x90 };

                static patch_t l_gamepadMappingsPatch9;
                l_gamepadMappingsPatch9 =
                    patchCreate( 0x4A02F2, l_gamepadMappingsPatch,
                            sizeof( l_gamepadMappingsPatch ) );
            }
        }

        // Disable keyboard mappings
        {
            {
                uint8_t l_keyboardMappingsPatch[] = {
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

                static patch_t l_keyboardMappingsPatch1;
                l_keyboardMappingsPatch1 =
                    patchCreate( 0x54D2C0, l_keyboardMappingsPatch,
                            sizeof( l_keyboardMappingsPatch ) );
            }
        }

        // Enable disabled stages
        {
            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch1;
                l_disabledStagePatch1 =
                    patchCreate( 0x54CEBC, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch2;
                l_disabledStagePatch2 =
                    patchCreate( 0x54CEC0, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch3;
                l_disabledStagePatch3 =
                    patchCreate( 0x54CEC4, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch4;
                l_disabledStagePatch4 =
                    patchCreate( 0x54CFA8, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch5;
                l_disabledStagePatch5 =
                    patchCreate( 0x54CFAC, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch6;
                l_disabledStagePatch6 =
                    patchCreate( 0x54CFB0, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch7;
                l_disabledStagePatch7 =
                    patchCreate( 0x54CF68, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch8;
                l_disabledStagePatch8 =
                    patchCreate( 0x54CF6C, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch9;
                l_disabledStagePatch9 =
                    patchCreate( 0x54CF70, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch10;
                l_disabledStagePatch10 =
                    patchCreate( 0x54CF74, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch11;
                l_disabledStagePatch11 =
                    patchCreate( 0x54CF78, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch12;
                l_disabledStagePatch12 =
                    patchCreate( 0x54CF7C, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch13;
                l_disabledStagePatch13 =
                    patchCreate( 0x54CF80, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch14;
                l_disabledStagePatch14 =
                    patchCreate( 0x54CF84, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch15;
                l_disabledStagePatch15 =
                    patchCreate( 0x54CF88, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch16;
                l_disabledStagePatch16 =
                    patchCreate( 0x54CF8C, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch17;
                l_disabledStagePatch17 =
                    patchCreate( 0x54CF90, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch18;
                l_disabledStagePatch18 =
                    patchCreate( 0x54CF94, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch19;
                l_disabledStagePatch19 =
                    patchCreate( 0x54CF98, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch20;
                l_disabledStagePatch20 =
                    patchCreate( 0x54CF9C, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch21;
                l_disabledStagePatch21 =
                    patchCreate( 0x54CFA0, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }

            {
                uint8_t l_disabledStagePatch[] = { 0xFF, 0x00, 0x00, 0x00 };

                static patch_t l_disabledStagePatch22;
                l_disabledStagePatch22 =
                    patchCreate( 0x54CFA4, l_disabledStagePatch,
                            sizeof( l_disabledStagePatch ) );
            }
        }

        // Disables the code that sets the intro state to 0. This is so we can
        // manually set it during rollback
        {
            uint8_t l_autoIntroStatePatch[] = { 0x90, 0x90, 0x90, 0x90,
                0x90, 0x90, 0x90 };

            static patch_t l_autoIntroStatePatch1;
            l_autoIntroStatePatch1 = patchCreate(
                    0x45C1F2, l_autoIntroStatePatch, sizeof( l_autoIntroStatePatch ) );
        }

        // Prevent training mode music from reseting
        {
            uint8_t l_trainingMusicResetPatch[] = { // jmp 00472C74
                0xEB, 0x05 };

            static patch_t l_trainingMusicResetPatch1;
            l_trainingMusicResetPatch1 =
                patchCreate( 0x472C6D, l_trainingMusicResetPatch,
                        sizeof( l_trainingMusicResetPatch ) );
        }

        {
            uint8_t l_bossStageFlashPatch[] = { 0xFF, 0xFF, 0xFF, 0xFF,
                0xFF, 0xFF, 0xFF, 0xFF,
                0xFF, 0xFF, 0xFF, 0xFF };

            static patch_t l_bossStageFlashPatch1;
            l_bossStageFlashPatch1 = patchCreate(
                    0x53B3C8, l_bossStageFlashPatch, sizeof( l_bossStageFlashPatch ) );
        }

        {
            uint8_t l_multipleWindowsPatch[] = { // jmp 0040D262
                0xEB };
            static patch_t l_multipleWindowsPatch1;
            l_multipleWindowsPatch1 =
                patchCreate( 0x40D25A, l_multipleWindowsPatch,
                        sizeof( l_multipleWindowsPatch ) );
        }

        // Skip MBAACC initial config window
        {
            {
                uint8_t l_initialConfigWindowPatch[] = { 0xEB, 0x0E };
                static patch_t l_initialConfigWindowPatch1;
                l_initialConfigWindowPatch1 = patchCreate( 0x04A1D42, l_initialConfigWindowPatch,
                        ( sizeof( l_initialConfigWindowPatch ) ) );
            }

            {
                uint8_t l_initialConfigWindowPatch[] = { 0xEB };
                static patch_t l_initialConfigWindowPatch2;
                l_initialConfigWindowPatch2 = patchCreate( 0x04A1D4A, l_initialConfigWindowPatch,
                        ( sizeof( l_initialConfigWindowPatch ) ) );
            }
        }

#undef INLINE_DWORD
    }

    return ( l_returnValue );
}
