#pragma once

class m_IDirect3DDevice9Ex : public IDirect3DDevice9Ex {
private:
    LPDIRECT3DDEVICE9EX ProxyInterface;
    m_IDirect3D9Ex* m_pD3DEx;
    REFIID WrapperID;

public:
    m_IDirect3DDevice9Ex( LPDIRECT3DDEVICE9EX pDevice,
                          m_IDirect3D9Ex* pD3D,
                          REFIID DeviceID = IID_IUnknown )
        : ProxyInterface( pDevice ), m_pD3DEx( pD3D ), WrapperID( DeviceID ) {
        InitDirect3DDevice();
    }
    void InitDirect3DDevice( void ) {
        ProxyAddressLookupTable =
            new AddressLookupTable< m_IDirect3DDevice9Ex >( this );
    }
    ~m_IDirect3DDevice9Ex() { delete ProxyAddressLookupTable; }

    LPDIRECT3DDEVICE9EX GetProxyInterface( void ) { return ProxyInterface; }
    AddressLookupTable< m_IDirect3DDevice9Ex >* ProxyAddressLookupTable;

    /*** IUnknown methods ***/
    STDMETHOD( QueryInterface )( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_( unsigned long, AddRef )( THIS );
    STDMETHOD_( unsigned long, Release )( THIS );

    /*** IDirect3DDevice9 methods ***/
    STDMETHOD( TestCooperativeLevel )( THIS );
    STDMETHOD_( unsigned int, GetAvailableTextureMem )( THIS );
    STDMETHOD( EvictManagedResources )( THIS );
    STDMETHOD( GetDirect3D )( THIS_ IDirect3D9** ppD3D9 );
    STDMETHOD( GetDeviceCaps )( THIS_ D3DCAPS9* pCaps );
    STDMETHOD( GetDisplayMode )( THIS_ unsigned int iSwapChain, D3DDISPLAYMODE* pMode );
    STDMETHOD( GetCreationParameters )
    ( THIS_ D3DDEVICE_CREATION_PARAMETERS* pParameters );
    STDMETHOD( SetCursorProperties )
    ( THIS_ unsigned int XHotSpot, unsigned int YHotSpot, IDirect3DSurface9* pCursorBitmap );
    STDMETHOD_( void, SetCursorPosition )( THIS_ int X, int Y, unsigned long Flags );
    STDMETHOD_( int, ShowCursor )( THIS_ int bShow );
    STDMETHOD( CreateAdditionalSwapChain )
    ( THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,
      IDirect3DSwapChain9** pSwapChain );
    STDMETHOD( GetSwapChain )
    ( THIS_ unsigned int iSwapChain, IDirect3DSwapChain9** pSwapChain );
    STDMETHOD_( unsigned int, GetNumberOfSwapChains )( THIS );
    STDMETHOD( Reset )( THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters );
    STDMETHOD( Present )
    ( THIS_ CONST RECT* pSourceRect,
      CONST RECT* pDestRect,
      HWND hDestWindowOverride,
      CONST RGNDATA* pDirtyRegion );
    STDMETHOD( GetBackBuffer )
    ( THIS_ unsigned int iSwapChain,
      unsigned int iBackBuffer,
      D3DBACKBUFFER_TYPE Type,
      IDirect3DSurface9** ppBackBuffer );
    STDMETHOD( GetRasterStatus )
    ( THIS_ unsigned int iSwapChain, D3DRASTER_STATUS* pRasterStatus );
    STDMETHOD( SetDialogBoxMode )( THIS_ int bEnableDialogs );
    STDMETHOD_( void, SetGammaRamp )
    ( THIS_ unsigned int iSwapChain, unsigned long Flags, CONST D3DGAMMARAMP* pRamp );
    STDMETHOD_( void, GetGammaRamp )
    ( THIS_ unsigned int iSwapChain, D3DGAMMARAMP* pRamp );
    STDMETHOD( CreateTexture )
    ( THIS_ unsigned int Width,
      unsigned int Height,
      unsigned int Levels,
      unsigned long Usage,
      D3DFORMAT Format,
      D3DPOOL Pool,
      IDirect3DTexture9** ppTexture,
      HANDLE* pSharedHandle );
    STDMETHOD( CreateVolumeTexture )
    ( THIS_ unsigned int Width,
      unsigned int Height,
      unsigned int Depth,
      unsigned int Levels,
      unsigned long Usage,
      D3DFORMAT Format,
      D3DPOOL Pool,
      IDirect3DVolumeTexture9** ppVolumeTexture,
      HANDLE* pSharedHandle );
    STDMETHOD( CreateCubeTexture )
    ( THIS_ unsigned int EdgeLength,
      unsigned int Levels,
      unsigned long Usage,
      D3DFORMAT Format,
      D3DPOOL Pool,
      IDirect3DCubeTexture9** ppCubeTexture,
      HANDLE* pSharedHandle );
    STDMETHOD( CreateVertexBuffer )
    ( THIS_ unsigned int Length,
      unsigned long Usage,
      unsigned long FVF,
      D3DPOOL Pool,
      IDirect3DVertexBuffer9** ppVertexBuffer,
      HANDLE* pSharedHandle );
    STDMETHOD( CreateIndexBuffer )
    ( THIS_ unsigned int Length,
      unsigned long Usage,
      D3DFORMAT Format,
      D3DPOOL Pool,
      IDirect3DIndexBuffer9** ppIndexBuffer,
      HANDLE* pSharedHandle );
    STDMETHOD( CreateRenderTarget )
    ( THIS_ unsigned int Width,
      unsigned int Height,
      D3DFORMAT Format,
      D3DMULTISAMPLE_TYPE MultiSample,
      unsigned long MultisampleQuality,
      int Lockable,
      IDirect3DSurface9** ppSurface,
      HANDLE* pSharedHandle );
    STDMETHOD( CreateDepthStencilSurface )
    ( THIS_ unsigned int Width,
      unsigned int Height,
      D3DFORMAT Format,
      D3DMULTISAMPLE_TYPE MultiSample,
      unsigned long MultisampleQuality,
      int Discard,
      IDirect3DSurface9** ppSurface,
      HANDLE* pSharedHandle );
    STDMETHOD( UpdateSurface )
    ( THIS_ IDirect3DSurface9* pSourceSurface,
      CONST RECT* pSourceRect,
      IDirect3DSurface9* pDestinationSurface,
      CONST POINT* pDestPoint );
    STDMETHOD( UpdateTexture )
    ( THIS_ IDirect3DBaseTexture9* pSourceTexture,
      IDirect3DBaseTexture9* pDestinationTexture );
    STDMETHOD( GetRenderTargetData )
    ( THIS_ IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface );
    STDMETHOD( GetFrontBufferData )
    ( THIS_ unsigned int iSwapChain, IDirect3DSurface9* pDestSurface );
    STDMETHOD( StretchRect )
    ( THIS_ IDirect3DSurface9* pSourceSurface,
      CONST RECT* pSourceRect,
      IDirect3DSurface9* pDestSurface,
      CONST RECT* pDestRect,
      D3DTEXTUREFILTERTYPE Filter );
    STDMETHOD( ColorFill )
    ( THIS_ IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color );
    STDMETHOD( CreateOffscreenPlainSurface )
    ( THIS_ unsigned int Width,
      unsigned int Height,
      D3DFORMAT Format,
      D3DPOOL Pool,
      IDirect3DSurface9** ppSurface,
      HANDLE* pSharedHandle );
    STDMETHOD( SetRenderTarget )
    ( THIS_ unsigned long RenderTargetIndex, IDirect3DSurface9* pRenderTarget );
    STDMETHOD( GetRenderTarget )
    ( THIS_ unsigned long RenderTargetIndex, IDirect3DSurface9** ppRenderTarget );
    STDMETHOD( SetDepthStencilSurface )
    ( THIS_ IDirect3DSurface9* pNewZStencil );
    STDMETHOD( GetDepthStencilSurface )
    ( THIS_ IDirect3DSurface9** ppZStencilSurface );
    STDMETHOD( BeginScene )( THIS );
    STDMETHOD( EndScene )( THIS );
    STDMETHOD( Clear )
    ( THIS_ unsigned long Count,
      CONST D3DRECT* pRects,
      unsigned long Flags,
      D3DCOLOR Color,
      float Z,
      unsigned long Stencil );
    STDMETHOD( SetTransform )
    ( THIS_ D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix );
    STDMETHOD( GetTransform )
    ( THIS_ D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix );
    STDMETHOD( MultiplyTransform )
    ( THIS_ D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix );
    STDMETHOD( SetViewport )( THIS_ CONST D3DVIEWPORT9* pViewport );
    STDMETHOD( GetViewport )( THIS_ D3DVIEWPORT9* pViewport );
    STDMETHOD( SetMaterial )( THIS_ CONST D3DMATERIAL9* pMaterial );
    STDMETHOD( GetMaterial )( THIS_ D3DMATERIAL9* pMaterial );
    STDMETHOD( SetLight )( THIS_ unsigned long Index, CONST D3DLIGHT9* pLight );
    STDMETHOD( GetLight )( THIS_ unsigned long Index, D3DLIGHT9* pLight );
    STDMETHOD( LightEnable )( THIS_ unsigned long Index, int Enable );
    STDMETHOD( GetLightEnable )( THIS_ unsigned long Index, int* pEnable );
    STDMETHOD( SetClipPlane )( THIS_ unsigned long Index, CONST float* pPlane );
    STDMETHOD( GetClipPlane )( THIS_ unsigned long Index, float* pPlane );
    STDMETHOD( SetRenderState )( THIS_ D3DRENDERSTATETYPE State, unsigned long Value );
    STDMETHOD( GetRenderState )
    ( THIS_ D3DRENDERSTATETYPE State, unsigned long* pValue );
    STDMETHOD( CreateStateBlock )
    ( THIS_ D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB );
    STDMETHOD( BeginStateBlock )( THIS );
    STDMETHOD( EndStateBlock )( THIS_ IDirect3DStateBlock9** ppSB );
    STDMETHOD( SetClipStatus )( THIS_ CONST D3DCLIPSTATUS9* pClipStatus );
    STDMETHOD( GetClipStatus )( THIS_ D3DCLIPSTATUS9* pClipStatus );
    STDMETHOD( GetTexture )
    ( THIS_ unsigned long Stage, IDirect3DBaseTexture9** ppTexture );
    STDMETHOD( SetTexture )
    ( THIS_ unsigned long Stage, IDirect3DBaseTexture9* pTexture );
    STDMETHOD( GetTextureStageState )
    ( THIS_ unsigned long Stage, D3DTEXTURESTAGESTATETYPE Type, unsigned long* pValue );
    STDMETHOD( SetTextureStageState )
    ( THIS_ unsigned long Stage, D3DTEXTURESTAGESTATETYPE Type, unsigned long Value );
    STDMETHOD( GetSamplerState )
    ( THIS_ unsigned long Sampler, D3DSAMPLERSTATETYPE Type, unsigned long* pValue );
    STDMETHOD( SetSamplerState )
    ( THIS_ unsigned long Sampler, D3DSAMPLERSTATETYPE Type, unsigned long Value );
    STDMETHOD( ValidateDevice )( THIS_ unsigned long* pNumPasses );
    STDMETHOD( SetPaletteEntries )
    ( THIS_ unsigned int PaletteNumber, CONST PALETTEENTRY* pEntries );
    STDMETHOD( GetPaletteEntries )
    ( THIS_ unsigned int PaletteNumber, PALETTEENTRY* pEntries );
    STDMETHOD( SetCurrentTexturePalette )( THIS_ unsigned int PaletteNumber );
    STDMETHOD( GetCurrentTexturePalette )( THIS_ unsigned int* PaletteNumber );
    STDMETHOD( SetScissorRect )( THIS_ CONST RECT* pRect );
    STDMETHOD( GetScissorRect )( THIS_ RECT* pRect );
    STDMETHOD( SetSoftwareVertexProcessing )( THIS_ int bSoftware );
    STDMETHOD_( int, GetSoftwareVertexProcessing )( THIS );
    STDMETHOD( SetNPatchMode )( THIS_ float nSegments );
    STDMETHOD_( float, GetNPatchMode )( THIS );
    STDMETHOD( DrawPrimitive )
    ( THIS_ D3DPRIMITIVETYPE PrimitiveType,
      unsigned int StartVertex,
      unsigned int PrimitiveCount );
    STDMETHOD( DrawIndexedPrimitive )
    ( THIS_ D3DPRIMITIVETYPE,
      INT BaseVertexIndex,
      unsigned int MinVertexIndex,
      unsigned int NumVertices,
      unsigned int startIndex,
      unsigned int primCount );
    STDMETHOD( DrawPrimitiveUP )
    ( THIS_ D3DPRIMITIVETYPE PrimitiveType,
      unsigned int PrimitiveCount,
      CONST void* pVertexStreamZeroData,
      unsigned int VertexStreamZeroStride );
    STDMETHOD( DrawIndexedPrimitiveUP )
    ( THIS_ D3DPRIMITIVETYPE PrimitiveType,
      unsigned int MinVertexIndex,
      unsigned int NumVertices,
      unsigned int PrimitiveCount,
      CONST void* pIndexData,
      D3DFORMAT IndexDataFormat,
      CONST void* pVertexStreamZeroData,
      unsigned int VertexStreamZeroStride );
    STDMETHOD( ProcessVertices )
    ( THIS_ unsigned int SrcStartIndex,
      unsigned int DestIndex,
      unsigned int VertexCount,
      IDirect3DVertexBuffer9* pDestBuffer,
      IDirect3DVertexDeclaration9* pVertexDecl,
      unsigned long Flags );
    STDMETHOD( CreateVertexDeclaration )
    ( THIS_ CONST D3DVERTEXELEMENT9* pVertexElements,
      IDirect3DVertexDeclaration9** ppDecl );
    STDMETHOD( SetVertexDeclaration )
    ( THIS_ IDirect3DVertexDeclaration9* pDecl );
    STDMETHOD( GetVertexDeclaration )
    ( THIS_ IDirect3DVertexDeclaration9** ppDecl );
    STDMETHOD( SetFVF )( THIS_ unsigned long FVF );
    STDMETHOD( GetFVF )( THIS_ unsigned long* pFVF );
    STDMETHOD( CreateVertexShader )
    ( THIS_ CONST unsigned long* pFunction, IDirect3DVertexShader9** ppShader );
    STDMETHOD( SetVertexShader )( THIS_ IDirect3DVertexShader9* pShader );
    STDMETHOD( GetVertexShader )( THIS_ IDirect3DVertexShader9** ppShader );
    STDMETHOD( SetVertexShaderConstantF )
    ( THIS_ unsigned int StartRegister,
      CONST float* pConstantData,
      unsigned int Vector4fCount );
    STDMETHOD( GetVertexShaderConstantF )
    ( THIS_ unsigned int StartRegister, float* pConstantData, unsigned int Vector4fCount );
    STDMETHOD( SetVertexShaderConstantI )
    ( THIS_ unsigned int StartRegister, CONST int* pConstantData, unsigned int Vector4iCount );
    STDMETHOD( GetVertexShaderConstantI )
    ( THIS_ unsigned int StartRegister, int* pConstantData, unsigned int Vector4iCount );
    STDMETHOD( SetVertexShaderConstantB )
    ( THIS_ unsigned int StartRegister, CONST int* pConstantData, unsigned int BoolCount );
    STDMETHOD( GetVertexShaderConstantB )
    ( THIS_ unsigned int StartRegister, int* pConstantData, unsigned int BoolCount );
    STDMETHOD( SetStreamSource )
    ( THIS_ unsigned int StreamNumber,
      IDirect3DVertexBuffer9* pStreamData,
      unsigned int OffsetInBytes,
      unsigned int Stride );
    STDMETHOD( GetStreamSource )
    ( THIS_ unsigned int StreamNumber,
      IDirect3DVertexBuffer9** ppStreamData,
      unsigned int* pOffsetInBytes,
      unsigned int* pStride );
    STDMETHOD( SetStreamSourceFreq )( THIS_ unsigned int StreamNumber, unsigned int Setting );
    STDMETHOD( GetStreamSourceFreq )( THIS_ unsigned int StreamNumber, unsigned int* pSetting );
    STDMETHOD( SetIndices )( THIS_ IDirect3DIndexBuffer9* pIndexData );
    STDMETHOD( GetIndices )( THIS_ IDirect3DIndexBuffer9** ppIndexData );
    STDMETHOD( CreatePixelShader )
    ( THIS_ CONST unsigned long* pFunction, IDirect3DPixelShader9** ppShader );
    STDMETHOD( SetPixelShader )( THIS_ IDirect3DPixelShader9* pShader );
    STDMETHOD( GetPixelShader )( THIS_ IDirect3DPixelShader9** ppShader );
    STDMETHOD( SetPixelShaderConstantF )
    ( THIS_ unsigned int StartRegister,
      CONST float* pConstantData,
      unsigned int Vector4fCount );
    STDMETHOD( GetPixelShaderConstantF )
    ( THIS_ unsigned int StartRegister, float* pConstantData, unsigned int Vector4fCount );
    STDMETHOD( SetPixelShaderConstantI )
    ( THIS_ unsigned int StartRegister, CONST int* pConstantData, unsigned int Vector4iCount );
    STDMETHOD( GetPixelShaderConstantI )
    ( THIS_ unsigned int StartRegister, int* pConstantData, unsigned int Vector4iCount );
    STDMETHOD( SetPixelShaderConstantB )
    ( THIS_ unsigned int StartRegister, CONST int* pConstantData, unsigned int BoolCount );
    STDMETHOD( GetPixelShaderConstantB )
    ( THIS_ unsigned int StartRegister, int* pConstantData, unsigned int BoolCount );
    STDMETHOD( DrawRectPatch )
    ( THIS_ unsigned int Handle,
      CONST float* pNumSegs,
      CONST D3DRECTPATCH_INFO* pRectPatchInfo );
    STDMETHOD( DrawTriPatch )
    ( THIS_ unsigned int Handle,
      CONST float* pNumSegs,
      CONST D3DTRIPATCH_INFO* pTriPatchInfo );
    STDMETHOD( DeletePatch )( THIS_ unsigned int Handle );
    STDMETHOD( CreateQuery )
    ( THIS_ D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery );
    STDMETHOD( SetConvolutionMonoKernel )
    ( THIS_ unsigned int width, unsigned int height, float* rows, float* columns );
    STDMETHOD( ComposeRects )
    ( THIS_ IDirect3DSurface9* pSrc,
      IDirect3DSurface9* pDst,
      IDirect3DVertexBuffer9* pSrcRectDescs,
      unsigned int NumRects,
      IDirect3DVertexBuffer9* pDstRectDescs,
      D3DCOMPOSERECTSOP Operation,
      int Xoffset,
      int Yoffset );
    STDMETHOD( PresentEx )
    ( THIS_ CONST RECT* pSourceRect,
      CONST RECT* pDestRect,
      HWND hDestWindowOverride,
      CONST RGNDATA* pDirtyRegion,
      unsigned long dwFlags );
    STDMETHOD( GetGPUThreadPriority )( THIS_ INT* pPriority );
    STDMETHOD( SetGPUThreadPriority )( THIS_ INT Priority );
    STDMETHOD( WaitForVBlank )( THIS_ unsigned int iSwapChain );
    STDMETHOD( CheckResourceResidency )
    ( THIS_ IDirect3DResource9** pResourceArray, unsigned int32 NumResources );
    STDMETHOD( SetMaximumFrameLatency )( THIS_ unsigned int MaxLatency );
    STDMETHOD( GetMaximumFrameLatency )( THIS_ unsigned int* pMaxLatency );
    STDMETHOD( CheckDeviceState )( THIS_ HWND hDestinationWindow );
    STDMETHOD( CreateRenderTargetEx )
    ( THIS_ unsigned int Width,
      unsigned int Height,
      D3DFORMAT Format,
      D3DMULTISAMPLE_TYPE MultiSample,
      unsigned long MultisampleQuality,
      int Lockable,
      IDirect3DSurface9** ppSurface,
      HANDLE* pSharedHandle,
      unsigned long Usage );
    STDMETHOD( CreateOffscreenPlainSurfaceEx )
    ( THIS_ unsigned int Width,
      unsigned int Height,
      D3DFORMAT Format,
      D3DPOOL Pool,
      IDirect3DSurface9** ppSurface,
      HANDLE* pSharedHandle,
      unsigned long Usage );
    STDMETHOD( CreateDepthStencilSurfaceEx )
    ( THIS_ unsigned int Width,
      unsigned int Height,
      D3DFORMAT Format,
      D3DMULTISAMPLE_TYPE MultiSample,
      unsigned long MultisampleQuality,
      int Discard,
      IDirect3DSurface9** ppSurface,
      HANDLE* pSharedHandle,
      unsigned long Usage );
    STDMETHOD( ResetEx )
    ( THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,
      D3DDISPLAYMODEEX* pFullscreenDisplayMode );
    STDMETHOD( GetDisplayModeEx )
    ( THIS_ unsigned int iSwapChain,
      D3DDISPLAYMODEEX* pMode,
      D3DDISPLAYROTATION* pRotation );
};
