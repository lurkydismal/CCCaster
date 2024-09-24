#pragma once

#include "d3d9.h"

typedef HRESULT( __attribute__( ( stdcall ) )* Direct3DCreate9ExProc )( UINT, IDirect3D9Ex** );

extern Direct3DCreate9ExProc m_pDirect3DCreate9Ex;

long __attribute__( ( stdcall ) ) Direct3DCreate9Ex( unsigned int SDKVersion, IDirect3D9Ex** ppD3D );
