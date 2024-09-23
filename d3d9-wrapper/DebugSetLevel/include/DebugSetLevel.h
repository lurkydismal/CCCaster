#pragma once

#include "d3d9.h"

typedef long( __attribute__( ( stdcall ) )* DebugSetLevelProc )( unsigned long );

extern DebugSetLevelProc m_pDebugSetLevel;

long __attribute__( ( stdcall ) ) DebugSetLevel( unsigned long _level );
