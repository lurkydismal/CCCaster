#pragma once

#include "d3d9.h"

typedef int( __attribute__( ( stdcall ) ) * D3DPERF_EndEventProc )( void );

extern D3DPERF_EndEventProc m_pD3DPERF_EndEvent;

int __attribute__( ( stdcall ) ) D3DPERF_EndEvent( void );
