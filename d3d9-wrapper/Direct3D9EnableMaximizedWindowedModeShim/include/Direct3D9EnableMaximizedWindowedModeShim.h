#pragma once

#include "d3d9.h"

typedef int( __attribute__( ( stdcall ) )* Direct3D9EnableMaximizedWindowedModeShimProc )( int _enable );

extern Direct3D9EnableMaximizedWindowedModeShimProc
    m_pDirect3D9EnableMaximizedWindowedModeShim;

int __attribute__( ( stdcall ) ) Direct3D9EnableMaximizedWindowedModeShim( int _enable );
