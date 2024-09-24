#pragma once

#define INITGUID

#include <d3d9.h>

class m_IDirect3D9Ex;
class m_IDirect3DDevice9Ex;
class m_IDirect3DCubeTexture9;
class m_IDirect3DIndexBuffer9;
class m_IDirect3DPixelShader9;
class m_IDirect3DQuery9;
class m_IDirect3DStateBlock9;
class m_IDirect3DSurface9;
class m_IDirect3DSwapChain9Ex;
class m_IDirect3DTexture9;
class m_IDirect3DVertexBuffer9;
class m_IDirect3DVertexDeclaration9;
class m_IDirect3DVertexShader9;
class m_IDirect3DVolume9;
class m_IDirect3DVolumeTexture9;

#include "AddressLookupTable.h"

void genericQueryInterface( REFIID riid,
                            LPVOID* ppvObj,
                            m_IDirect3DDevice9Ex* m_pDeviceEx );

#include "IDirect3D9Ex.h"
#include "IDirect3DCubeTexture9.h"
#include "IDirect3DDevice9Ex.h"
#include "IDirect3DIndexBuffer9.h"
#include "IDirect3DPixelShader9.h"
#include "IDirect3DQuery9.h"
#include "IDirect3DStateBlock9.h"
#include "IDirect3DSurface9.h"
#include "IDirect3DSwapChain9Ex.h"
#include "IDirect3DTexture9.h"
#include "IDirect3DVertexBuffer9.h"
#include "IDirect3DVertexDeclaration9.h"
#include "IDirect3DVertexShader9.h"
#include "IDirect3DVolume9.h"
#include "IDirect3DVolumeTexture9.h"
