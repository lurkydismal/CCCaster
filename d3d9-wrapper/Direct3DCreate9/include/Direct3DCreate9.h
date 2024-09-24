#pragma once

#include "d3d9.h"

typedef IDirect3D9*( __attribute__( ( stdcall ) )* Direct3DCreate9Proc )( unsigned int );

extern Direct3DCreate9Proc m_pDirect3DCreate9;

IDirect3D9* __attribute__( ( stdcall ) ) Direct3DCreate9( unsigned int SDKVersion );
