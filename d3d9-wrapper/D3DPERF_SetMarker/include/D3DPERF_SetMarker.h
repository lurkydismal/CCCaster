#pragma once

#include "d3d9.h"

typedef void( __attribute__( ( stdcall ) ) *
              D3DPERF_SetMarkerProc )( D3DCOLOR _color, LPCWSTR _eventName );

extern D3DPERF_SetMarkerProc m_pD3DPERF_SetMarker;

void __attribute__( ( stdcall ) ) D3DPERF_SetMarker( D3DCOLOR _color,
                                                     LPCWSTR _eventName );
