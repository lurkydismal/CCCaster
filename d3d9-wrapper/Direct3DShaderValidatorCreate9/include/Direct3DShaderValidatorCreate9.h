#pragma once

#include "d3d9.h"

typedef long( __attribute__( ( stdcall ) ) *
              Direct3DShaderValidatorCreate9Proc )( void );

extern Direct3DShaderValidatorCreate9Proc m_pDirect3DShaderValidatorCreate9;

long __declspec( dllexport ) __attribute__( ( stdcall ) )
Direct3DShaderValidatorCreate9( void );
