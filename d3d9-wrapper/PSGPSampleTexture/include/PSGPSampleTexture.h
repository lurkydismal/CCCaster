#pragma once

#include "d3d9.h"

typedef long( __attribute__( ( stdcall ) )* PSGPSampleTextureProc )( void );

extern PSGPSampleTextureProc m_pPSGPSampleTexture;

long __attribute__( ( stdcall ) ) PSGPSampleTexture( void );
