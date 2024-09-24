#pragma once

#include "d3d9.h"

typedef void( __attribute__( ( stdcall ) ) *
              D3DPERF_SetOptionsProc )( unsigned long _options );

extern D3DPERF_SetOptionsProc m_pD3DPERF_SetOptions;

void __attribute__( ( stdcall ) ) D3DPERF_SetOptions( unsigned long _options );
