#pragma once

#include "d3d9.h"

typedef unsigned long( __attribute__( ( stdcall ) )* D3DPERF_GetStatusProc )( void );

extern D3DPERF_GetStatusProc m_pD3DPERF_GetStatus;

unsigned long __attribute__( ( stdcall ) ) D3DPERF_GetStatus( void );
