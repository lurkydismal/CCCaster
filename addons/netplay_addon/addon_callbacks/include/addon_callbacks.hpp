#pragma once

#include <cstdint>

enum AVAILABLE_FLAGS_NETPLAY { SHOW_OVERLAY_NETPLAY = 0b1 };

extern "C" {

uint16_t __declspec( dllexport ) gameMode$opening( void** _callbackArguments );

uint16_t __declspec( dllexport ) gameMode$characterSelect(
    void** _callbackArguments );

uint16_t __declspec( dllexport ) gameMode$loading( void** _callbackArguments );

uint16_t __declspec( dllexport ) gameMode$inMatch( void** _callbackArguments );

uint16_t __declspec( dllexport ) keyboard$getLocalInput$end(
    void** _callbackArguments );

uint16_t __declspec( dllexport ) overlay$beforeDraw$ImGui(
    void** _callbackArguments );

uint16_t __declspec( dllexport ) mainLoop$getLocalInput(
    void** _callbackArguments );
}
