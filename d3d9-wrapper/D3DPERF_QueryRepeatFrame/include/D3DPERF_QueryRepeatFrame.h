#pragma once

#include "d3d9.h"

typedef int( __attribute__( ( stdcall ) ) *
             D3DPERF_QueryRepeatFrameProc )( void );

extern D3DPERF_QueryRepeatFrameProc m_pD3DPERF_QueryRepeatFrame;

int __attribute__( ( stdcall ) ) D3DPERF_QueryRepeatFrame( void );
