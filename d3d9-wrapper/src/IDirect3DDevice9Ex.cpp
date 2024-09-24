#include <omp.h>

#include "_useCallback.h"
#include "_useCallback.hpp"
#include "d3d9.h"

static size_t l_renderCallsPerFrame = 0;

long m_IDirect3DDevice9Ex::Present( const RECT* pSourceRect,
                                    const RECT* pDestRect,
                                    HWND hDestWindowOverride,
                                    const RGNDATA* pDirtyRegion ) {
    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$Present", pSourceRect, pDestRect,
                      hDestWindowOverride, pDirtyRegion );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    l_renderCallsPerFrame = 0;

    return ( ProxyInterface->Present( pSourceRect, pDestRect,
                                      hDestWindowOverride, pDirtyRegion ) );
}

long m_IDirect3DDevice9Ex::PresentEx( THIS_ const RECT* pSourceRect,
                                      const RECT* pDestRect,
                                      HWND hDestWindowOverride,
                                      const RGNDATA* pDirtyRegion,
                                      unsigned long dwFlags ) {
    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$PresentEx", pSourceRect, pDestRect,
                      hDestWindowOverride, pDirtyRegion, &dwFlags );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( ProxyInterface->PresentEx(
        pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags ) );
}

long m_IDirect3DDevice9Ex::EndScene( void ) {
    l_renderCallsPerFrame++;

    uint16_t l_result = _useCallback( "IDirect3DDevice9Ex$EndScene" );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( ProxyInterface->EndScene() );
}

long m_IDirect3D9Ex::CreateDevice(
    unsigned int Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    unsigned long BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    IDirect3DDevice9** ppReturnedDeviceInterface ) {
    long hr = ProxyInterface->CreateDevice(
        Adapter, DeviceType, hFocusWindow, BehaviorFlags,
        pPresentationParameters, ppReturnedDeviceInterface );

    if ( SUCCEEDED( hr ) && ppReturnedDeviceInterface ) {
        *ppReturnedDeviceInterface = new m_IDirect3DDevice9Ex(
            ( IDirect3DDevice9Ex* )*ppReturnedDeviceInterface, this,
            IID_IDirect3DDevice9 );
    }

#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] =
        "IDirect3D9Ex CreateDevice ( Adapter, DeviceType, hFocusWindow, "
        "BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface)\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    uint16_t l_result = _useCallback(
        "IDirect3D9Ex$CreateDevice", &Adapter, &DeviceType, &hFocusWindow,
        &BehaviorFlags, &pPresentationParameters, &ppReturnedDeviceInterface );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::Reset(
    D3DPRESENT_PARAMETERS* pPresentationParameters ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] =
        "m_IDirect3DDevice9Ex Reset ( pPresentationParameters )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$PreReset", pPresentationParameters );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    long hRet = ProxyInterface->Reset( pPresentationParameters );

    l_result =
        _useCallback( "IDirect3DDevice9Ex$PostReset", pPresentationParameters );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hRet );
}

long m_IDirect3D9Ex::CreateDeviceEx(
    THIS_ unsigned int Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    unsigned long BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode,
    IDirect3DDevice9Ex** ppReturnedDeviceInterface ) {
    long hr = ProxyInterface->CreateDeviceEx(
        Adapter, DeviceType, hFocusWindow, BehaviorFlags,
        pPresentationParameters, pFullscreenDisplayMode,
        ppReturnedDeviceInterface );

    if ( SUCCEEDED( hr ) && ppReturnedDeviceInterface ) {
        *ppReturnedDeviceInterface = new m_IDirect3DDevice9Ex(
            *ppReturnedDeviceInterface, this, IID_IDirect3DDevice9Ex );
    }

#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] =
        "IDirect3D9Ex CreateDeviceEx ( Adapter, DeviceType, hFocusWindow, "
        "BehaviorFlags, pPresentationParameters, pFullscreenDisplayMode, "
        "ppReturnedDeviceInterface )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    uint16_t l_result =
        _useCallback( "IDirect3D9Ex$CreateDeviceEx", &Adapter, &DeviceType,
                      hFocusWindow, &BehaviorFlags, pPresentationParameters,
                      pFullscreenDisplayMode, ppReturnedDeviceInterface );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::ResetEx(
    THIS_ D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode ) {
#if defined( LOG_EXPORTED_CALLS )

    const char l_message[] =
        "m_IDirect3DDevice9Ex ResetEx ( pPresentationParameters, "
        "pFullscreenDisplayMode )\n";

    _useCallback( "log$transaction$query", l_message );
    _useCallback( "log$transaction$commit" );

#endif // LOG_EXPORTED_CALLS

    uint16_t l_result =
        _useCallback( "IDirect3DDevice9Ex$PreResetEx", pPresentationParameters,
                      pFullscreenDisplayMode );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    long hRet = ProxyInterface->ResetEx( pPresentationParameters,
                                         pFullscreenDisplayMode );

    l_result = _useCallback( "IDirect3DDevice9Ex$PostResetEx",
                             pPresentationParameters, pFullscreenDisplayMode );

    if ( ( l_result != 0 ) && ( l_result != ENODATA ) ) {
        return ( E_FAIL );
    }

    return ( hRet );
}

long m_IDirect3DDevice9Ex::QueryInterface( REFIID riid, void** ppvObj ) {
    if ( ( ( riid == IID_IUnknown ) || ( riid == WrapperID ) ) && ( ppvObj ) ) {
        AddRef();

        *ppvObj = this;

        return ( S_OK );
    }

    long hr = ProxyInterface->QueryInterface( riid, ppvObj );

    if ( SUCCEEDED( hr ) ) {
        genericQueryInterface( riid, ppvObj, this );
    }

    return ( hr );
}

unsigned long m_IDirect3DDevice9Ex::AddRef( void ) {
    return ( ProxyInterface->AddRef() );
}

unsigned long m_IDirect3DDevice9Ex::Release( void ) {
    unsigned long count = ProxyInterface->Release();

    if ( count == 0 ) {
        delete this;
    }

    return ( count );
}

void m_IDirect3DDevice9Ex::SetCursorPosition( int X,
                                              int Y,
                                              unsigned long Flags ) {
    return ( ProxyInterface->SetCursorPosition( X, Y, Flags ) );
}

long m_IDirect3DDevice9Ex::SetCursorProperties(
    unsigned int XHotSpot,
    unsigned int YHotSpot,
    IDirect3DSurface9* pCursorBitmap ) {
    if ( pCursorBitmap ) {
        pCursorBitmap = static_cast< m_IDirect3DSurface9* >( pCursorBitmap )
                            ->GetProxyInterface();
    }

    return ( ProxyInterface->SetCursorProperties( XHotSpot, YHotSpot,
                                                  pCursorBitmap ) );
}

int m_IDirect3DDevice9Ex::ShowCursor( int bShow ) {
    return ( ProxyInterface->ShowCursor( bShow ) );
}

long m_IDirect3DDevice9Ex::CreateAdditionalSwapChain(
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    IDirect3DSwapChain9** ppSwapChain ) {
    long hr = ProxyInterface->CreateAdditionalSwapChain(
        pPresentationParameters, ppSwapChain );

    if ( ( SUCCEEDED( hr ) ) && ( ppSwapChain ) ) {
        *ppSwapChain = new m_IDirect3DSwapChain9Ex(
            ( IDirect3DSwapChain9Ex* )*ppSwapChain, this );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::CreateCubeTexture(
    THIS_ unsigned int EdgeLength,
    unsigned int Levels,
    unsigned long Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    IDirect3DCubeTexture9** ppCubeTexture,
    HANDLE* pSharedHandle ) {
    long hr = ProxyInterface->CreateCubeTexture(
        EdgeLength, Levels, Usage, Format, Pool, ppCubeTexture, pSharedHandle );

    if ( ( SUCCEEDED( hr ) ) && ( ppCubeTexture ) ) {
        *ppCubeTexture = new m_IDirect3DCubeTexture9( *ppCubeTexture, this );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::CreateDepthStencilSurface(
    THIS_ unsigned int Width,
    unsigned int Height,
    D3DFORMAT Format,
    D3DMULTISAMPLE_TYPE MultiSample,
    unsigned long MultisampleQuality,
    int Discard,
    IDirect3DSurface9** ppSurface,
    HANDLE* pSharedHandle ) {
    long hr = ProxyInterface->CreateDepthStencilSurface(
        Width, Height, Format, MultiSample, MultisampleQuality, Discard,
        ppSurface, pSharedHandle );

    if ( ( SUCCEEDED( hr ) ) && ( ppSurface ) ) {
        *ppSurface = new m_IDirect3DSurface9( *ppSurface, this );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::CreateIndexBuffer(
    THIS_ unsigned int Length,
    unsigned long Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    IDirect3DIndexBuffer9** ppIndexBuffer,
    HANDLE* pSharedHandle ) {
    long hr = ProxyInterface->CreateIndexBuffer( Length, Usage, Format, Pool,
                                                 ppIndexBuffer, pSharedHandle );

    if ( ( SUCCEEDED( hr ) ) && ( ppIndexBuffer ) ) {
        *ppIndexBuffer = new m_IDirect3DIndexBuffer9( *ppIndexBuffer, this );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::CreateRenderTarget( THIS_ unsigned int Width,
                                               unsigned int Height,
                                               D3DFORMAT Format,
                                               D3DMULTISAMPLE_TYPE MultiSample,
                                               unsigned long MultisampleQuality,
                                               int Lockable,
                                               IDirect3DSurface9** ppSurface,
                                               HANDLE* pSharedHandle ) {
    long hr = ProxyInterface->CreateRenderTarget(
        Width, Height, Format, MultiSample, MultisampleQuality, Lockable,
        ppSurface, pSharedHandle );

    if ( ( SUCCEEDED( hr ) ) && ( ppSurface ) ) {
        *ppSurface = new m_IDirect3DSurface9( *ppSurface, this );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::CreateTexture( THIS_ unsigned int Width,
                                          unsigned int Height,
                                          unsigned int Levels,
                                          unsigned long Usage,
                                          D3DFORMAT Format,
                                          D3DPOOL Pool,
                                          IDirect3DTexture9** ppTexture,
                                          HANDLE* pSharedHandle ) {
    long hr = ProxyInterface->CreateTexture(
        Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle );

    if ( ( SUCCEEDED( hr ) ) && ( ppTexture ) ) {
        *ppTexture = new m_IDirect3DTexture9( *ppTexture, this );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::CreateVertexBuffer(
    THIS_ unsigned int Length,
    unsigned long Usage,
    unsigned long FVF,
    D3DPOOL Pool,
    IDirect3DVertexBuffer9** ppVertexBuffer,
    HANDLE* pSharedHandle ) {
    long hr = ProxyInterface->CreateVertexBuffer(
        Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle );

    if ( ( SUCCEEDED( hr ) ) && ( ppVertexBuffer ) ) {
        *ppVertexBuffer = new m_IDirect3DVertexBuffer9( *ppVertexBuffer, this );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::CreateVolumeTexture(
    THIS_ unsigned int Width,
    unsigned int Height,
    unsigned int Depth,
    unsigned int Levels,
    unsigned long Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    IDirect3DVolumeTexture9** ppVolumeTexture,
    HANDLE* pSharedHandle ) {
    long hr = ProxyInterface->CreateVolumeTexture(
        Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture,
        pSharedHandle );

    if ( ( SUCCEEDED( hr ) ) && ( ppVolumeTexture ) ) {
        *ppVolumeTexture =
            new m_IDirect3DVolumeTexture9( *ppVolumeTexture, this );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::BeginStateBlock( void ) {
    return ProxyInterface->BeginStateBlock();
}

long m_IDirect3DDevice9Ex::CreateStateBlock( THIS_ D3DSTATEBLOCKTYPE Type,
                                             IDirect3DStateBlock9** ppSB ) {
    long hr = ProxyInterface->CreateStateBlock( Type, ppSB );

    if ( ( SUCCEEDED( hr ) ) && ( ppSB ) ) {
        *ppSB = new m_IDirect3DStateBlock9( *ppSB, this );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::EndStateBlock( THIS_ IDirect3DStateBlock9** ppSB ) {
    long hr = ProxyInterface->EndStateBlock( ppSB );

    if ( ( SUCCEEDED( hr ) ) && ( ppSB ) ) {
        *ppSB = ProxyAddressLookupTable->FindAddress< m_IDirect3DStateBlock9 >(
            *ppSB );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::GetClipStatus( D3DCLIPSTATUS9* pClipStatus ) {
    return ( ProxyInterface->GetClipStatus( pClipStatus ) );
}

long m_IDirect3DDevice9Ex::GetDisplayMode( THIS_ unsigned int iSwapChain,
                                           D3DDISPLAYMODE* pMode ) {
    return ( ProxyInterface->GetDisplayMode( iSwapChain, pMode ) );
}

long m_IDirect3DDevice9Ex::GetRenderState( D3DRENDERSTATETYPE State,
                                           unsigned long* pValue ) {
    return ( ProxyInterface->GetRenderState( State, pValue ) );
}

long m_IDirect3DDevice9Ex::GetRenderTarget(
    THIS_ unsigned long RenderTargetIndex,
    IDirect3DSurface9** ppRenderTarget ) {
    long hr =
        ProxyInterface->GetRenderTarget( RenderTargetIndex, ppRenderTarget );

    if ( ( SUCCEEDED( hr ) ) && ( ppRenderTarget ) ) {
        *ppRenderTarget =
            ProxyAddressLookupTable->FindAddress< m_IDirect3DSurface9 >(
                *ppRenderTarget );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::GetTransform( D3DTRANSFORMSTATETYPE State,
                                         D3DMATRIX* pMatrix ) {
    return ( ProxyInterface->GetTransform( State, pMatrix ) );
}

long m_IDirect3DDevice9Ex::SetClipStatus( const D3DCLIPSTATUS9* pClipStatus ) {
    return ( ProxyInterface->SetClipStatus( pClipStatus ) );
}

long m_IDirect3DDevice9Ex::SetRenderState( D3DRENDERSTATETYPE State,
                                           unsigned long Value ) {
    return ( ProxyInterface->SetRenderState( State, Value ) );
}

long m_IDirect3DDevice9Ex::SetRenderTarget(
    THIS_ unsigned long RenderTargetIndex,
    IDirect3DSurface9* pRenderTarget ) {
    if ( pRenderTarget ) {
        pRenderTarget = static_cast< m_IDirect3DSurface9* >( pRenderTarget )
                            ->GetProxyInterface();
    }

    return (
        ProxyInterface->SetRenderTarget( RenderTargetIndex, pRenderTarget ) );
}

long m_IDirect3DDevice9Ex::SetTransform( D3DTRANSFORMSTATETYPE State,
                                         const D3DMATRIX* pMatrix ) {
    return ( ProxyInterface->SetTransform( State, pMatrix ) );
}

void m_IDirect3DDevice9Ex::GetGammaRamp( THIS_ unsigned int iSwapChain,
                                         D3DGAMMARAMP* pRamp ) {
    return ( ProxyInterface->GetGammaRamp( iSwapChain, pRamp ) );
}

void m_IDirect3DDevice9Ex::SetGammaRamp( THIS_ unsigned int iSwapChain,
                                         unsigned long Flags,
                                         const D3DGAMMARAMP* pRamp ) {
    return ( ProxyInterface->SetGammaRamp( iSwapChain, Flags, pRamp ) );
}

long m_IDirect3DDevice9Ex::DeletePatch( unsigned int Handle ) {
    return ( ProxyInterface->DeletePatch( Handle ) );
}

long m_IDirect3DDevice9Ex::DrawRectPatch(
    unsigned int Handle,
    const float* pNumSegs,
    const D3DRECTPATCH_INFO* pRectPatchInfo ) {
    return (
        ProxyInterface->DrawRectPatch( Handle, pNumSegs, pRectPatchInfo ) );
}

long m_IDirect3DDevice9Ex::DrawTriPatch(
    unsigned int Handle,
    const float* pNumSegs,
    const D3DTRIPATCH_INFO* pTriPatchInfo ) {
    return ( ProxyInterface->DrawTriPatch( Handle, pNumSegs, pTriPatchInfo ) );
}

long m_IDirect3DDevice9Ex::GetIndices(
    THIS_ IDirect3DIndexBuffer9** ppIndexData ) {
    long hr = ProxyInterface->GetIndices( ppIndexData );

    if ( ( SUCCEEDED( hr ) ) && ( ppIndexData ) ) {
        *ppIndexData =
            ProxyAddressLookupTable->FindAddress< m_IDirect3DIndexBuffer9 >(
                *ppIndexData );
    }

    return ( hr );
}

long m_IDirect3DDevice9Ex::SetIndices(
    THIS_ IDirect3DIndexBuffer9* pIndexData ) {
    if ( pIndexData ) {
        pIndexData = static_cast< m_IDirect3DIndexBuffer9* >( pIndexData )
                         ->GetProxyInterface();
    }

    return ( ProxyInterface->SetIndices( pIndexData ) );
}

unsigned int m_IDirect3DDevice9Ex::GetAvailableTextureMem( void ) {
    return ProxyInterface->GetAvailableTextureMem();
}

long m_IDirect3DDevice9Ex::GetCreationParameters(
    D3DDEVICE_CREATION_PARAMETERS* pParameters ) {
    return ProxyInterface->GetCreationParameters( pParameters );
}

long m_IDirect3DDevice9Ex::GetDeviceCaps( D3DCAPS9* pCaps ) {
    return ProxyInterface->GetDeviceCaps( pCaps );
}

long m_IDirect3DDevice9Ex::GetDirect3D( IDirect3D9** ppD3D9 ) {
    if ( ppD3D9 ) {
        m_pD3DEx->AddRef();

        *ppD3D9 = m_pD3DEx;

        return D3D_OK;
    }
    return D3DERR_INVALIDCALL;
}

long m_IDirect3DDevice9Ex::GetRasterStatus( THIS_ unsigned int iSwapChain,
                                            D3DRASTER_STATUS* pRasterStatus ) {
    return ProxyInterface->GetRasterStatus( iSwapChain, pRasterStatus );
}

long m_IDirect3DDevice9Ex::GetLight( unsigned long Index, D3DLIGHT9* pLight ) {
    return ProxyInterface->GetLight( Index, pLight );
}

long m_IDirect3DDevice9Ex::GetLightEnable( unsigned long Index, int* pEnable ) {
    return ProxyInterface->GetLightEnable( Index, pEnable );
}

long m_IDirect3DDevice9Ex::GetMaterial( D3DMATERIAL9* pMaterial ) {
    return ProxyInterface->GetMaterial( pMaterial );
}

long m_IDirect3DDevice9Ex::LightEnable( unsigned long LightIndex,
                                        int bEnable ) {
    return ProxyInterface->LightEnable( LightIndex, bEnable );
}

long m_IDirect3DDevice9Ex::SetLight( unsigned long Index,
                                     const D3DLIGHT9* pLight ) {
    return ProxyInterface->SetLight( Index, pLight );
}

long m_IDirect3DDevice9Ex::SetMaterial( const D3DMATERIAL9* pMaterial ) {
    return ProxyInterface->SetMaterial( pMaterial );
}

long m_IDirect3DDevice9Ex::MultiplyTransform( D3DTRANSFORMSTATETYPE State,
                                              const D3DMATRIX* pMatrix ) {
    return ProxyInterface->MultiplyTransform( State, pMatrix );
}

long m_IDirect3DDevice9Ex::ProcessVertices(
    THIS_ unsigned int SrcStartIndex,
    unsigned int DestIndex,
    unsigned int VertexCount,
    IDirect3DVertexBuffer9* pDestBuffer,
    IDirect3DVertexDeclaration9* pVertexDecl,
    unsigned long Flags ) {
    if ( pDestBuffer ) {
        pDestBuffer = static_cast< m_IDirect3DVertexBuffer9* >( pDestBuffer )
                          ->GetProxyInterface();
    }

    if ( pVertexDecl ) {
        pVertexDecl =
            static_cast< m_IDirect3DVertexDeclaration9* >( pVertexDecl )
                ->GetProxyInterface();
    }

    return ProxyInterface->ProcessVertices( SrcStartIndex, DestIndex,
                                            VertexCount, pDestBuffer,
                                            pVertexDecl, Flags );
}

long m_IDirect3DDevice9Ex::TestCooperativeLevel( void ) {
    return ProxyInterface->TestCooperativeLevel();
}

long m_IDirect3DDevice9Ex::GetCurrentTexturePalette(
    unsigned int* pPaletteNumber ) {
    return ProxyInterface->GetCurrentTexturePalette( pPaletteNumber );
}

long m_IDirect3DDevice9Ex::GetPaletteEntries( unsigned int PaletteNumber,
                                              PALETTEENTRY* pEntries ) {
    return ProxyInterface->GetPaletteEntries( PaletteNumber, pEntries );
}

long m_IDirect3DDevice9Ex::SetCurrentTexturePalette(
    unsigned int PaletteNumber ) {
    return ProxyInterface->SetCurrentTexturePalette( PaletteNumber );
}

long m_IDirect3DDevice9Ex::SetPaletteEntries( unsigned int PaletteNumber,
                                              const PALETTEENTRY* pEntries ) {
    return ProxyInterface->SetPaletteEntries( PaletteNumber, pEntries );
}

long m_IDirect3DDevice9Ex::CreatePixelShader(
    THIS_ const unsigned long* pFunction,
    IDirect3DPixelShader9** ppShader ) {
    long hr = ProxyInterface->CreatePixelShader( pFunction, ppShader );

    if ( SUCCEEDED( hr ) && ppShader ) {
        *ppShader = new m_IDirect3DPixelShader9( *ppShader, this );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::GetPixelShader(
    THIS_ IDirect3DPixelShader9** ppShader ) {
    long hr = ProxyInterface->GetPixelShader( ppShader );

    if ( SUCCEEDED( hr ) && ppShader ) {
        *ppShader =
            ProxyAddressLookupTable->FindAddress< m_IDirect3DPixelShader9 >(
                *ppShader );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::SetPixelShader(
    THIS_ IDirect3DPixelShader9* pShader ) {
    if ( pShader ) {
        pShader = static_cast< m_IDirect3DPixelShader9* >( pShader )
                      ->GetProxyInterface();
    }

    return ProxyInterface->SetPixelShader( pShader );
}

long m_IDirect3DDevice9Ex::DrawIndexedPrimitive( THIS_ D3DPRIMITIVETYPE Type,
                                                 int BaseVertexIndex,
                                                 unsigned int MinVertexIndex,
                                                 unsigned int NumVertices,
                                                 unsigned int startIndex,
                                                 unsigned int primCount ) {
    return ProxyInterface->DrawIndexedPrimitive( Type, BaseVertexIndex,
                                                 MinVertexIndex, NumVertices,
                                                 startIndex, primCount );
}

long m_IDirect3DDevice9Ex::DrawIndexedPrimitiveUP(
    D3DPRIMITIVETYPE PrimitiveType,
    unsigned int MinIndex,
    unsigned int NumVertices,
    unsigned int PrimitiveCount,
    const void* pIndexData,
    D3DFORMAT IndexDataFormat,
    const void* pVertexStreamZeroData,
    unsigned int VertexStreamZeroStride ) {
    return ProxyInterface->DrawIndexedPrimitiveUP(
        PrimitiveType, MinIndex, NumVertices, PrimitiveCount, pIndexData,
        IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride );
}

long m_IDirect3DDevice9Ex::DrawPrimitive( D3DPRIMITIVETYPE PrimitiveType,
                                          unsigned int StartVertex,
                                          unsigned int PrimitiveCount ) {
    return ProxyInterface->DrawPrimitive( PrimitiveType, StartVertex,
                                          PrimitiveCount );
}

long m_IDirect3DDevice9Ex::DrawPrimitiveUP(
    D3DPRIMITIVETYPE PrimitiveType,
    unsigned int PrimitiveCount,
    const void* pVertexStreamZeroData,
    unsigned int VertexStreamZeroStride ) {
    return ProxyInterface->DrawPrimitiveUP( PrimitiveType, PrimitiveCount,
                                            pVertexStreamZeroData,
                                            VertexStreamZeroStride );
}

long m_IDirect3DDevice9Ex::BeginScene( void ) {
    return ProxyInterface->BeginScene();
}

long m_IDirect3DDevice9Ex::GetStreamSource(
    THIS_ unsigned int StreamNumber,
    IDirect3DVertexBuffer9** ppStreamData,
    unsigned int* OffsetInBytes,
    unsigned int* pStride ) {
    long hr = ProxyInterface->GetStreamSource( StreamNumber, ppStreamData,
                                               OffsetInBytes, pStride );

    if ( SUCCEEDED( hr ) && ppStreamData ) {
        *ppStreamData =
            ProxyAddressLookupTable->FindAddress< m_IDirect3DVertexBuffer9 >(
                *ppStreamData );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::SetStreamSource( THIS_ unsigned int StreamNumber,
                                            IDirect3DVertexBuffer9* pStreamData,
                                            unsigned int OffsetInBytes,
                                            unsigned int Stride ) {
    if ( pStreamData ) {
        pStreamData = static_cast< m_IDirect3DVertexBuffer9* >( pStreamData )
                          ->GetProxyInterface();
    }

    return ProxyInterface->SetStreamSource( StreamNumber, pStreamData,
                                            OffsetInBytes, Stride );
}

long m_IDirect3DDevice9Ex::GetBackBuffer( THIS_ unsigned int iSwapChain,
                                          unsigned int iBackBuffer,
                                          D3DBACKBUFFER_TYPE Type,
                                          IDirect3DSurface9** ppBackBuffer ) {
    long hr = ProxyInterface->GetBackBuffer( iSwapChain, iBackBuffer, Type,
                                             ppBackBuffer );

    if ( SUCCEEDED( hr ) && ppBackBuffer ) {
        *ppBackBuffer =
            ProxyAddressLookupTable->FindAddress< m_IDirect3DSurface9 >(
                *ppBackBuffer );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::GetDepthStencilSurface(
    IDirect3DSurface9** ppZStencilSurface ) {
    long hr = ProxyInterface->GetDepthStencilSurface( ppZStencilSurface );

    if ( SUCCEEDED( hr ) && ppZStencilSurface ) {
        *ppZStencilSurface =
            ProxyAddressLookupTable->FindAddress< m_IDirect3DSurface9 >(
                *ppZStencilSurface );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::GetTexture( unsigned long Stage,
                                       IDirect3DBaseTexture9** ppTexture ) {
    long hr = ProxyInterface->GetTexture( Stage, ppTexture );

    if ( SUCCEEDED( hr ) && ppTexture && *ppTexture ) {
        switch ( ( *ppTexture )->GetType() ) {
            case D3DRTYPE_TEXTURE:
                *ppTexture =
                    ProxyAddressLookupTable->FindAddress< m_IDirect3DTexture9 >(
                        *ppTexture );
                break;
            case D3DRTYPE_VOLUMETEXTURE:
                *ppTexture = ProxyAddressLookupTable
                                 ->FindAddress< m_IDirect3DVolumeTexture9 >(
                                     *ppTexture );
                break;
            case D3DRTYPE_CUBETEXTURE:
                *ppTexture =
                    ProxyAddressLookupTable
                        ->FindAddress< m_IDirect3DCubeTexture9 >( *ppTexture );
                break;
            default:
                return D3DERR_INVALIDCALL;
        }
    }

    return hr;
}

long m_IDirect3DDevice9Ex::GetTextureStageState( unsigned long Stage,
                                                 D3DTEXTURESTAGESTATETYPE Type,
                                                 unsigned long* pValue ) {
    return ProxyInterface->GetTextureStageState( Stage, Type, pValue );
}

long m_IDirect3DDevice9Ex::SetTexture( unsigned long Stage,
                                       IDirect3DBaseTexture9* pTexture ) {
    if ( pTexture ) {
        switch ( pTexture->GetType() ) {
            case D3DRTYPE_TEXTURE:
                pTexture = static_cast< m_IDirect3DTexture9* >( pTexture )
                               ->GetProxyInterface();
                break;
            case D3DRTYPE_VOLUMETEXTURE:
                pTexture = static_cast< m_IDirect3DVolumeTexture9* >( pTexture )
                               ->GetProxyInterface();
                break;
            case D3DRTYPE_CUBETEXTURE:
                pTexture = static_cast< m_IDirect3DCubeTexture9* >( pTexture )
                               ->GetProxyInterface();
                break;
            default:
                return D3DERR_INVALIDCALL;
        }
    }

    return ProxyInterface->SetTexture( Stage, pTexture );
}

long m_IDirect3DDevice9Ex::SetTextureStageState( unsigned long Stage,
                                                 D3DTEXTURESTAGESTATETYPE Type,
                                                 unsigned long Value ) {
    return ProxyInterface->SetTextureStageState( Stage, Type, Value );
}

long m_IDirect3DDevice9Ex::UpdateTexture(
    IDirect3DBaseTexture9* pSourceTexture,
    IDirect3DBaseTexture9* pDestinationTexture ) {
    if ( pSourceTexture ) {
        switch ( pSourceTexture->GetType() ) {
            case D3DRTYPE_TEXTURE:
                pSourceTexture =
                    static_cast< m_IDirect3DTexture9* >( pSourceTexture )
                        ->GetProxyInterface();
                break;
            case D3DRTYPE_VOLUMETEXTURE:
                pSourceTexture =
                    static_cast< m_IDirect3DVolumeTexture9* >( pSourceTexture )
                        ->GetProxyInterface();
                break;
            case D3DRTYPE_CUBETEXTURE:
                pSourceTexture =
                    static_cast< m_IDirect3DCubeTexture9* >( pSourceTexture )
                        ->GetProxyInterface();
                break;
            default:
                return D3DERR_INVALIDCALL;
        }
    }
    if ( pDestinationTexture ) {
        switch ( pDestinationTexture->GetType() ) {
            case D3DRTYPE_TEXTURE:
                pDestinationTexture =
                    static_cast< m_IDirect3DTexture9* >( pDestinationTexture )
                        ->GetProxyInterface();
                break;
            case D3DRTYPE_VOLUMETEXTURE:
                pDestinationTexture = static_cast< m_IDirect3DVolumeTexture9* >(
                                          pDestinationTexture )
                                          ->GetProxyInterface();
                break;
            case D3DRTYPE_CUBETEXTURE:
                pDestinationTexture = static_cast< m_IDirect3DCubeTexture9* >(
                                          pDestinationTexture )
                                          ->GetProxyInterface();
                break;
            default:
                return D3DERR_INVALIDCALL;
        }
    }

    return ProxyInterface->UpdateTexture( pSourceTexture, pDestinationTexture );
}

long m_IDirect3DDevice9Ex::ValidateDevice( unsigned long* pNumPasses ) {
    return ProxyInterface->ValidateDevice( pNumPasses );
}

long m_IDirect3DDevice9Ex::GetClipPlane( unsigned long Index, float* pPlane ) {
    return ProxyInterface->GetClipPlane( Index, pPlane );
}

long m_IDirect3DDevice9Ex::SetClipPlane( unsigned long Index,
                                         const float* pPlane ) {
    return ProxyInterface->SetClipPlane( Index, pPlane );
}

long m_IDirect3DDevice9Ex::Clear( unsigned long Count,
                                  const D3DRECT* pRects,
                                  unsigned long Flags,
                                  D3DCOLOR Color,
                                  float Z,
                                  unsigned long Stencil ) {
    return ProxyInterface->Clear( Count, pRects, Flags, Color, Z, Stencil );
}

long m_IDirect3DDevice9Ex::GetViewport( D3DVIEWPORT9* pViewport ) {
    return ProxyInterface->GetViewport( pViewport );
}

long m_IDirect3DDevice9Ex::SetViewport( const D3DVIEWPORT9* pViewport ) {
    return ProxyInterface->SetViewport( pViewport );
}

long m_IDirect3DDevice9Ex::CreateVertexShader(
    THIS_ const unsigned long* pFunction,
    IDirect3DVertexShader9** ppShader ) {
    long hr = ProxyInterface->CreateVertexShader( pFunction, ppShader );

    if ( SUCCEEDED( hr ) && ppShader ) {
        *ppShader = new m_IDirect3DVertexShader9( *ppShader, this );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::GetVertexShader(
    THIS_ IDirect3DVertexShader9** ppShader ) {
    long hr = ProxyInterface->GetVertexShader( ppShader );

    if ( SUCCEEDED( hr ) && ppShader ) {
        *ppShader =
            ProxyAddressLookupTable->FindAddress< m_IDirect3DVertexShader9 >(
                *ppShader );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::SetVertexShader(
    THIS_ IDirect3DVertexShader9* pShader ) {
    if ( pShader ) {
        pShader = static_cast< m_IDirect3DVertexShader9* >( pShader )
                      ->GetProxyInterface();
    }

    return ProxyInterface->SetVertexShader( pShader );
}

long m_IDirect3DDevice9Ex::CreateQuery( THIS_ D3DQUERYTYPE Type,
                                        IDirect3DQuery9** ppQuery ) {
    long hr = ProxyInterface->CreateQuery( Type, ppQuery );

    if ( SUCCEEDED( hr ) && ppQuery ) {
        *ppQuery = new m_IDirect3DQuery9( *ppQuery, this );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::SetPixelShaderConstantB(
    THIS_ unsigned int StartRegister,
    const int* pConstantData,
    unsigned int BoolCount ) {
    return ProxyInterface->SetPixelShaderConstantB( StartRegister,
                                                    pConstantData, BoolCount );
}

long m_IDirect3DDevice9Ex::GetPixelShaderConstantB(
    THIS_ unsigned int StartRegister,
    int* pConstantData,
    unsigned int BoolCount ) {
    return ProxyInterface->GetPixelShaderConstantB( StartRegister,
                                                    pConstantData, BoolCount );
}

long m_IDirect3DDevice9Ex::SetPixelShaderConstantI(
    THIS_ unsigned int StartRegister,
    const int* pConstantData,
    unsigned int Vector4iCount ) {
    return ProxyInterface->SetPixelShaderConstantI(
        StartRegister, pConstantData, Vector4iCount );
}

long m_IDirect3DDevice9Ex::GetPixelShaderConstantI(
    THIS_ unsigned int StartRegister,
    int* pConstantData,
    unsigned int Vector4iCount ) {
    return ProxyInterface->GetPixelShaderConstantI(
        StartRegister, pConstantData, Vector4iCount );
}

long m_IDirect3DDevice9Ex::SetPixelShaderConstantF(
    THIS_ unsigned int StartRegister,
    const float* pConstantData,
    unsigned int Vector4fCount ) {
    return ProxyInterface->SetPixelShaderConstantF(
        StartRegister, pConstantData, Vector4fCount );
}

long m_IDirect3DDevice9Ex::GetPixelShaderConstantF(
    THIS_ unsigned int StartRegister,
    float* pConstantData,
    unsigned int Vector4fCount ) {
    return ProxyInterface->GetPixelShaderConstantF(
        StartRegister, pConstantData, Vector4fCount );
}

long m_IDirect3DDevice9Ex::SetStreamSourceFreq( THIS_ unsigned int StreamNumber,
                                                unsigned int Divider ) {
    return ProxyInterface->SetStreamSourceFreq( StreamNumber, Divider );
}

long m_IDirect3DDevice9Ex::GetStreamSourceFreq( THIS_ unsigned int StreamNumber,
                                                unsigned int* Divider ) {
    return ProxyInterface->GetStreamSourceFreq( StreamNumber, Divider );
}

long m_IDirect3DDevice9Ex::SetVertexShaderConstantB(
    THIS_ unsigned int StartRegister,
    const int* pConstantData,
    unsigned int BoolCount ) {
    return ProxyInterface->SetVertexShaderConstantB( StartRegister,
                                                     pConstantData, BoolCount );
}

long m_IDirect3DDevice9Ex::GetVertexShaderConstantB(
    THIS_ unsigned int StartRegister,
    int* pConstantData,
    unsigned int BoolCount ) {
    return ProxyInterface->GetVertexShaderConstantB( StartRegister,
                                                     pConstantData, BoolCount );
}

long m_IDirect3DDevice9Ex::SetVertexShaderConstantF(
    THIS_ unsigned int StartRegister,
    const float* pConstantData,
    unsigned int Vector4fCount ) {
    return ProxyInterface->SetVertexShaderConstantF(
        StartRegister, pConstantData, Vector4fCount );
}

long m_IDirect3DDevice9Ex::GetVertexShaderConstantF(
    THIS_ unsigned int StartRegister,
    float* pConstantData,
    unsigned int Vector4fCount ) {
    return ProxyInterface->GetVertexShaderConstantF(
        StartRegister, pConstantData, Vector4fCount );
}

long m_IDirect3DDevice9Ex::SetVertexShaderConstantI(
    THIS_ unsigned int StartRegister,
    const int* pConstantData,
    unsigned int Vector4iCount ) {
    return ProxyInterface->SetVertexShaderConstantI(
        StartRegister, pConstantData, Vector4iCount );
}

long m_IDirect3DDevice9Ex::GetVertexShaderConstantI(
    THIS_ unsigned int StartRegister,
    int* pConstantData,
    unsigned int Vector4iCount ) {
    return ProxyInterface->GetVertexShaderConstantI(
        StartRegister, pConstantData, Vector4iCount );
}

long m_IDirect3DDevice9Ex::SetFVF( THIS_ unsigned long FVF ) {
    return ProxyInterface->SetFVF( FVF );
}

long m_IDirect3DDevice9Ex::GetFVF( THIS_ unsigned long* pFVF ) {
    return ProxyInterface->GetFVF( pFVF );
}

long m_IDirect3DDevice9Ex::CreateVertexDeclaration(
    THIS_ const D3DVERTEXELEMENT9* pVertexElements,
    IDirect3DVertexDeclaration9** ppDecl ) {
    long hr =
        ProxyInterface->CreateVertexDeclaration( pVertexElements, ppDecl );

    if ( SUCCEEDED( hr ) && ppDecl ) {
        *ppDecl = new m_IDirect3DVertexDeclaration9( *ppDecl, this );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::SetVertexDeclaration(
    THIS_ IDirect3DVertexDeclaration9* pDecl ) {
    if ( pDecl ) {
        pDecl = static_cast< m_IDirect3DVertexDeclaration9* >( pDecl )
                    ->GetProxyInterface();
    }

    return ProxyInterface->SetVertexDeclaration( pDecl );
}

long m_IDirect3DDevice9Ex::GetVertexDeclaration(
    THIS_ IDirect3DVertexDeclaration9** ppDecl ) {
    long hr = ProxyInterface->GetVertexDeclaration( ppDecl );

    if ( SUCCEEDED( hr ) && ppDecl ) {
        *ppDecl = ProxyAddressLookupTable
                      ->FindAddress< m_IDirect3DVertexDeclaration9 >( *ppDecl );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::SetNPatchMode( THIS_ float nSegments ) {
    return ProxyInterface->SetNPatchMode( nSegments );
}

float m_IDirect3DDevice9Ex::GetNPatchMode( THIS ) {
    return ProxyInterface->GetNPatchMode();
}

int m_IDirect3DDevice9Ex::GetSoftwareVertexProcessing( THIS ) {
    return ProxyInterface->GetSoftwareVertexProcessing();
}

unsigned int m_IDirect3DDevice9Ex::GetNumberOfSwapChains( THIS ) {
    return ProxyInterface->GetNumberOfSwapChains();
}

long m_IDirect3DDevice9Ex::EvictManagedResources( THIS ) {
    return ProxyInterface->EvictManagedResources();
}

long m_IDirect3DDevice9Ex::SetSoftwareVertexProcessing( THIS_ int bSoftware ) {
    return ProxyInterface->SetSoftwareVertexProcessing( bSoftware );
}

long m_IDirect3DDevice9Ex::SetScissorRect( THIS_ const RECT* pRect ) {
    return ProxyInterface->SetScissorRect( pRect );
}

long m_IDirect3DDevice9Ex::GetScissorRect( THIS_ RECT* pRect ) {
    return ProxyInterface->GetScissorRect( pRect );
}

long m_IDirect3DDevice9Ex::GetSamplerState( THIS_ unsigned long Sampler,
                                            D3DSAMPLERSTATETYPE Type,
                                            unsigned long* pValue ) {
    return ProxyInterface->GetSamplerState( Sampler, Type, pValue );
}

long m_IDirect3DDevice9Ex::SetSamplerState( THIS_ unsigned long Sampler,
                                            D3DSAMPLERSTATETYPE Type,
                                            unsigned long Value ) {
    return ProxyInterface->SetSamplerState( Sampler, Type, Value );
}

long m_IDirect3DDevice9Ex::SetDepthStencilSurface(
    THIS_ IDirect3DSurface9* pNewZStencil ) {
    if ( pNewZStencil ) {
        pNewZStencil = static_cast< m_IDirect3DSurface9* >( pNewZStencil )
                           ->GetProxyInterface();
    }

    return ProxyInterface->SetDepthStencilSurface( pNewZStencil );
}

long m_IDirect3DDevice9Ex::CreateOffscreenPlainSurface(
    THIS_ unsigned int Width,
    unsigned int Height,
    D3DFORMAT Format,
    D3DPOOL Pool,
    IDirect3DSurface9** ppSurface,
    HANDLE* pSharedHandle ) {
    long hr = ProxyInterface->CreateOffscreenPlainSurface(
        Width, Height, Format, Pool, ppSurface, pSharedHandle );

    if ( SUCCEEDED( hr ) && ppSurface ) {
        *ppSurface = new m_IDirect3DSurface9( *ppSurface, this );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::ColorFill( THIS_ IDirect3DSurface9* pSurface,
                                      const RECT* pRect,
                                      D3DCOLOR color ) {
    if ( pSurface ) {
        pSurface = static_cast< m_IDirect3DSurface9* >( pSurface )
                       ->GetProxyInterface();
    }

    return ProxyInterface->ColorFill( pSurface, pRect, color );
}

long m_IDirect3DDevice9Ex::StretchRect( THIS_ IDirect3DSurface9* pSourceSurface,
                                        const RECT* pSourceRect,
                                        IDirect3DSurface9* pDestSurface,
                                        const RECT* pDestRect,
                                        D3DTEXTUREFILTERTYPE Filter ) {
    if ( pSourceSurface ) {
        pSourceSurface = static_cast< m_IDirect3DSurface9* >( pSourceSurface )
                             ->GetProxyInterface();
    }

    if ( pDestSurface ) {
        pDestSurface = static_cast< m_IDirect3DSurface9* >( pDestSurface )
                           ->GetProxyInterface();
    }

    return ProxyInterface->StretchRect( pSourceSurface, pSourceRect,
                                        pDestSurface, pDestRect, Filter );
}

long m_IDirect3DDevice9Ex::GetFrontBufferData(
    THIS_ unsigned int iSwapChain,
    IDirect3DSurface9* pDestSurface ) {
    if ( pDestSurface ) {
        pDestSurface = static_cast< m_IDirect3DSurface9* >( pDestSurface )
                           ->GetProxyInterface();
    }

    return ProxyInterface->GetFrontBufferData( iSwapChain, pDestSurface );
}

long m_IDirect3DDevice9Ex::GetRenderTargetData(
    THIS_ IDirect3DSurface9* pRenderTarget,
    IDirect3DSurface9* pDestSurface ) {
    if ( pRenderTarget ) {
        pRenderTarget = static_cast< m_IDirect3DSurface9* >( pRenderTarget )
                            ->GetProxyInterface();
    }

    if ( pDestSurface ) {
        pDestSurface = static_cast< m_IDirect3DSurface9* >( pDestSurface )
                           ->GetProxyInterface();
    }

    return ProxyInterface->GetRenderTargetData( pRenderTarget, pDestSurface );
}

long m_IDirect3DDevice9Ex::UpdateSurface(
    THIS_ IDirect3DSurface9* pSourceSurface,
    const RECT* pSourceRect,
    IDirect3DSurface9* pDestinationSurface,
    const POINT* pDestPoint ) {
    if ( pSourceSurface ) {
        pSourceSurface = static_cast< m_IDirect3DSurface9* >( pSourceSurface )
                             ->GetProxyInterface();
    }

    if ( pDestinationSurface ) {
        pDestinationSurface =
            static_cast< m_IDirect3DSurface9* >( pDestinationSurface )
                ->GetProxyInterface();
    }

    return ProxyInterface->UpdateSurface( pSourceSurface, pSourceRect,
                                          pDestinationSurface, pDestPoint );
}

long m_IDirect3DDevice9Ex::SetDialogBoxMode( THIS_ int bEnableDialogs ) {
    return ProxyInterface->SetDialogBoxMode( bEnableDialogs );
}

long m_IDirect3DDevice9Ex::GetSwapChain( THIS_ unsigned int iSwapChain,
                                         IDirect3DSwapChain9** ppSwapChain ) {
    long hr = ProxyInterface->GetSwapChain( iSwapChain, ppSwapChain );

    if ( SUCCEEDED( hr ) && ppSwapChain ) {
        *ppSwapChain =
            ProxyAddressLookupTable->FindAddress< m_IDirect3DSwapChain9Ex >(
                *ppSwapChain );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::SetConvolutionMonoKernel( THIS_ unsigned int width,
                                                     unsigned int height,
                                                     float* rows,
                                                     float* columns ) {
    return ProxyInterface->SetConvolutionMonoKernel( width, height, rows,
                                                     columns );
}

long m_IDirect3DDevice9Ex::ComposeRects( THIS_ IDirect3DSurface9* pSrc,
                                         IDirect3DSurface9* pDst,
                                         IDirect3DVertexBuffer9* pSrcRectDescs,
                                         unsigned int NumRects,
                                         IDirect3DVertexBuffer9* pDstRectDescs,
                                         D3DCOMPOSERECTSOP Operation,
                                         int Xoffset,
                                         int Yoffset ) {
    if ( pSrc ) {
        pSrc = static_cast< m_IDirect3DSurface9* >( pSrc )->GetProxyInterface();
    }

    if ( pDst ) {
        pDst = static_cast< m_IDirect3DSurface9* >( pDst )->GetProxyInterface();
    }

    if ( pSrcRectDescs ) {
        pSrcRectDescs =
            static_cast< m_IDirect3DVertexBuffer9* >( pSrcRectDescs )
                ->GetProxyInterface();
    }

    if ( pDstRectDescs ) {
        pDstRectDescs =
            static_cast< m_IDirect3DVertexBuffer9* >( pDstRectDescs )
                ->GetProxyInterface();
    }

    return ProxyInterface->ComposeRects( pSrc, pDst, pSrcRectDescs, NumRects,
                                         pDstRectDescs, Operation, Xoffset,
                                         Yoffset );
}

long m_IDirect3DDevice9Ex::GetGPUThreadPriority( THIS_ int* pPriority ) {
    return ProxyInterface->GetGPUThreadPriority( pPriority );
}

long m_IDirect3DDevice9Ex::SetGPUThreadPriority( THIS_ int Priority ) {
    return ProxyInterface->SetGPUThreadPriority( Priority );
}

long m_IDirect3DDevice9Ex::WaitForVBlank( THIS_ unsigned int iSwapChain ) {
    return ProxyInterface->WaitForVBlank( iSwapChain );
}

long m_IDirect3DDevice9Ex::CheckResourceResidency(
    THIS_ IDirect3DResource9** pResourceArray,
    unsigned int NumResources ) {
    if ( pResourceArray ) {
        for ( unsigned int i = 0; i < NumResources; i++ ) {
            if ( pResourceArray[ i ] ) {
                switch ( pResourceArray[ i ]->GetType() ) {
                    case D3DRTYPE_SURFACE:
                        pResourceArray[ i ] =
                            static_cast< m_IDirect3DSurface9* >(
                                pResourceArray[ i ] )
                                ->GetProxyInterface();
                        break;
                    case D3DRTYPE_VOLUME:
                        pResourceArray[ i ] =
                            ( IDirect3DResource9* )reinterpret_cast<
                                m_IDirect3DVolume9* >( pResourceArray[ i ] )
                                ->GetProxyInterface();
                        break;
                    case D3DRTYPE_TEXTURE:
                        pResourceArray[ i ] =
                            static_cast< m_IDirect3DTexture9* >(
                                pResourceArray[ i ] )
                                ->GetProxyInterface();
                        break;
                    case D3DRTYPE_VOLUMETEXTURE:
                        pResourceArray[ i ] =
                            static_cast< m_IDirect3DVolumeTexture9* >(
                                pResourceArray[ i ] )
                                ->GetProxyInterface();
                        break;
                    case D3DRTYPE_CUBETEXTURE:
                        pResourceArray[ i ] =
                            static_cast< m_IDirect3DCubeTexture9* >(
                                pResourceArray[ i ] )
                                ->GetProxyInterface();
                        break;
                    case D3DRTYPE_VERTEXBUFFER:
                        pResourceArray[ i ] =
                            static_cast< m_IDirect3DVertexBuffer9* >(
                                pResourceArray[ i ] )
                                ->GetProxyInterface();
                        break;
                    case D3DRTYPE_INDEXBUFFER:
                        pResourceArray[ i ] =
                            static_cast< m_IDirect3DIndexBuffer9* >(
                                pResourceArray[ i ] )
                                ->GetProxyInterface();
                        break;
                    default:
                        return ( D3DERR_INVALIDCALL );
                }
            }
        }
    }

    return ProxyInterface->CheckResourceResidency( pResourceArray,
                                                   NumResources );
}

long m_IDirect3DDevice9Ex::SetMaximumFrameLatency(
    THIS_ unsigned int MaxLatency ) {
    return ProxyInterface->SetMaximumFrameLatency( MaxLatency );
}

long m_IDirect3DDevice9Ex::GetMaximumFrameLatency(
    THIS_ unsigned int* pMaxLatency ) {
    return ProxyInterface->GetMaximumFrameLatency( pMaxLatency );
}

long m_IDirect3DDevice9Ex::CheckDeviceState( THIS_ HWND hDestinationWindow ) {
    return ProxyInterface->CheckDeviceState( hDestinationWindow );
}

long m_IDirect3DDevice9Ex::CreateRenderTargetEx(
    THIS_ unsigned int Width,
    unsigned int Height,
    D3DFORMAT Format,
    D3DMULTISAMPLE_TYPE MultiSample,
    unsigned long MultisampleQuality,
    int Lockable,
    IDirect3DSurface9** ppSurface,
    HANDLE* pSharedHandle,
    unsigned long Usage ) {
    long hr = ProxyInterface->CreateRenderTargetEx(
        Width, Height, Format, MultiSample, MultisampleQuality, Lockable,
        ppSurface, pSharedHandle, Usage );

    if ( SUCCEEDED( hr ) && ppSurface ) {
        *ppSurface = new m_IDirect3DSurface9( *ppSurface, this );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::CreateOffscreenPlainSurfaceEx(
    THIS_ unsigned int Width,
    unsigned int Height,
    D3DFORMAT Format,
    D3DPOOL Pool,
    IDirect3DSurface9** ppSurface,
    HANDLE* pSharedHandle,
    unsigned long Usage ) {
    long hr = ProxyInterface->CreateOffscreenPlainSurfaceEx(
        Width, Height, Format, Pool, ppSurface, pSharedHandle, Usage );

    if ( SUCCEEDED( hr ) && ppSurface ) {
        *ppSurface = new m_IDirect3DSurface9( *ppSurface, this );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::CreateDepthStencilSurfaceEx(
    THIS_ unsigned int Width,
    unsigned int Height,
    D3DFORMAT Format,
    D3DMULTISAMPLE_TYPE MultiSample,
    unsigned long MultisampleQuality,
    int Discard,
    IDirect3DSurface9** ppSurface,
    HANDLE* pSharedHandle,
    unsigned long Usage ) {
    long hr = ProxyInterface->CreateDepthStencilSurfaceEx(
        Width, Height, Format, MultiSample, MultisampleQuality, Discard,
        ppSurface, pSharedHandle, Usage );

    if ( SUCCEEDED( hr ) && ppSurface ) {
        *ppSurface = new m_IDirect3DSurface9( *ppSurface, this );
    }

    return hr;
}

long m_IDirect3DDevice9Ex::GetDisplayModeEx( THIS_ unsigned int iSwapChain,
                                             D3DDISPLAYMODEEX* pMode,
                                             D3DDISPLAYROTATION* pRotation ) {
    return ProxyInterface->GetDisplayModeEx( iSwapChain, pMode, pRotation );
}
