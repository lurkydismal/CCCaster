#pragma once

#include "d3d9.h"

typedef void( __attribute__( ( stdcall ) )* D3DPERF_SetRegionProc )( D3DCOLOR _color, LPCWSTR _eventName );

extern D3DPERF_SetRegionProc m_pD3DPERF_SetRegion;

void __attribute__( ( stdcall ) ) D3DPERF_SetRegion( D3DCOLOR _color, LPCWSTR _eventName );
