#pragma once

#include "d3d9.h"

typedef int( __attribute__( ( stdcall ) )* D3DPERF_BeginEventProc )( D3DCOLOR, LPCWSTR );

extern D3DPERF_BeginEventProc m_pD3DPERF_BeginEvent;

int __attribute__( ( stdcall ) ) D3DPERF_BeginEvent( D3DCOLOR _color, LPCWSTR _eventName );
