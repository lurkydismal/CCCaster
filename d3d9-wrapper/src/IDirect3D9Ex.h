#pragma once

class m_IDirect3D9Ex : public IDirect3D9Ex {
private:
    LPDIRECT3D9EX ProxyInterface;
    REFIID WrapperID;

public:
    m_IDirect3D9Ex( LPDIRECT3D9EX pDirect3D, REFIID DeviceID )
        : ProxyInterface( pDirect3D ), WrapperID( DeviceID ) {}

    /*** IUnknown methods ***/
    STDMETHOD( QueryInterface )( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_( unsigned long, AddRef )( THIS );
    STDMETHOD_( unsigned long, Release )( THIS );

    /*** IDirect3D9 methods ***/
    STDMETHOD( RegisterSoftwareDevice )( THIS_ void* pInitializeFunction );
    STDMETHOD_( unsigned int, GetAdapterCount )( THIS );
    STDMETHOD( GetAdapterIdentifier )
    ( THIS_ unsigned int Adapter,
      unsigned long Flags,
      D3DADAPTER_IDENTIFIER9* pIdentifier );
    STDMETHOD_( unsigned int, GetAdapterModeCount )
    ( THIS_ unsigned int Adapter, D3DFORMAT Format );
    STDMETHOD( EnumAdapterModes )
    ( THIS_ unsigned int Adapter,
      D3DFORMAT Format,
      unsigned int Mode,
      D3DDISPLAYMODE* pMode );
    STDMETHOD( GetAdapterDisplayMode )
    ( THIS_ unsigned int Adapter, D3DDISPLAYMODE* pMode );
    STDMETHOD( CheckDeviceType )
    ( THIS_ unsigned int Adapter,
      D3DDEVTYPE DevType,
      D3DFORMAT AdapterFormat,
      D3DFORMAT BackBufferFormat,
      int bWindowed );
    STDMETHOD( CheckDeviceFormat )
    ( THIS_ unsigned int Adapter,
      D3DDEVTYPE DeviceType,
      D3DFORMAT AdapterFormat,
      unsigned long Usage,
      D3DRESOURCETYPE RType,
      D3DFORMAT CheckFormat );
    STDMETHOD( CheckDeviceMultiSampleType )
    ( THIS_ unsigned int Adapter,
      D3DDEVTYPE DeviceType,
      D3DFORMAT SurfaceFormat,
      int Windowed,
      D3DMULTISAMPLE_TYPE MultiSampleType,
      unsigned long* pQualityLevels );
    STDMETHOD( CheckDepthStencilMatch )
    ( THIS_ unsigned int Adapter,
      D3DDEVTYPE DeviceType,
      D3DFORMAT AdapterFormat,
      D3DFORMAT RenderTargetFormat,
      D3DFORMAT DepthStencilFormat );
    STDMETHOD( CheckDeviceFormatConversion )
    ( THIS_ unsigned int Adapter,
      D3DDEVTYPE DeviceType,
      D3DFORMAT SourceFormat,
      D3DFORMAT TargetFormat );
    STDMETHOD( GetDeviceCaps )
    ( THIS_ unsigned int Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps );
    STDMETHOD_( HMONITOR, GetAdapterMonitor )( THIS_ unsigned int Adapter );
    STDMETHOD( CreateDevice )
    ( THIS_ unsigned int Adapter,
      D3DDEVTYPE DeviceType,
      HWND hFocusWindow,
      unsigned long BehaviorFlags,
      D3DPRESENT_PARAMETERS* pPresentationParameters,
      IDirect3DDevice9** ppReturnedDeviceInterface );
    STDMETHOD_( unsigned int, GetAdapterModeCountEx )
    ( THIS_ unsigned int Adapter, CONST D3DDISPLAYMODEFILTER* pFilter );
    STDMETHOD( EnumAdapterModesEx )
    ( THIS_ unsigned int Adapter,
      CONST D3DDISPLAYMODEFILTER* pFilter,
      unsigned int Mode,
      D3DDISPLAYMODEEX* pMode );
    STDMETHOD( GetAdapterDisplayModeEx )
    ( THIS_ unsigned int Adapter,
      D3DDISPLAYMODEEX* pMode,
      D3DDISPLAYROTATION* pRotation );
    STDMETHOD( CreateDeviceEx )
    ( THIS_ unsigned int Adapter,
      D3DDEVTYPE DeviceType,
      HWND hFocusWindow,
      unsigned long BehaviorFlags,
      D3DPRESENT_PARAMETERS* pPresentationParameters,
      D3DDISPLAYMODEEX* pFullscreenDisplayMode,
      IDirect3DDevice9Ex** ppReturnedDeviceInterface );
    STDMETHOD( GetAdapterLUID )( THIS_ unsigned int Adapter, LUID* pLUID );
};
