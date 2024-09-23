#pragma once

#include "d3d9.h"

typedef void( __attribute__( ( stdcall ) )* DebugSetMuteProc )( void );

extern DebugSetMuteProc m_pDebugSetMute;

void __attribute__( ( stdcall ) ) DebugSetMute( void );
