#pragma once

#include "d3d9.h"

typedef long( __attribute__( ( stdcall ) ) * PSGPErrorProc )( void );

extern PSGPErrorProc m_pPSGPError;

long __declspec( dllexport ) __attribute__( ( stdcall ) ) PSGPError( void );
