#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>

extern HWND g_hFocusWindow;

extern "C" {

uint16_t __declspec( dllexport )
    IDirect3D9Ex$CreateDevice( void** _callbackArguments );
}
