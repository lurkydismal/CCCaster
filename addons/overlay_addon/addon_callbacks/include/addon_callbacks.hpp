#pragma once

#include <cstdint>

enum AVAILABLE_FLAGS { SHOW_OVERLAY = 0b1 };

extern "C" {

uint16_t __declspec( dllexport )
    IDirect3D9Ex$CreateDevice( void** _callbackArguments );

uint16_t __declspec( dllexport )
    IDirect3DDevice9Ex$EndScene( void** _callbackArguments );

uint16_t __declspec( dllexport )
    IDirect3DDevice9Ex$PreReset( void** _callbackArguments );

uint16_t __declspec( dllexport )
    IDirect3DDevice9Ex$PostReset( void** _callbackArguments );

uint16_t __declspec( dllexport )
    CustomWindowProcedure( void** _callbackArguments );

uint16_t __declspec( dllexport ) extraDrawCallback( void** _callbackArguments );

uint16_t __declspec( dllexport )
    mainLoop$getLocalInput( void** _callbackArguments );

uint16_t __declspec( dllexport ) overlay$Toggle( void );
}
