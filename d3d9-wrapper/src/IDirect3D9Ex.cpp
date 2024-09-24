#include "d3d9.h"

HRESULT m_IDirect3D9Ex::QueryInterface( REFIID riid, void** ppvObj ) {
    if ( ( riid == IID_IUnknown || riid == WrapperID ) && ppvObj ) {
        AddRef();

        *ppvObj = this;

        return ( S_OK );
    }

    return ( ProxyInterface->QueryInterface( riid, ppvObj ) );
}

unsigned long m_IDirect3D9Ex::AddRef( void ) {
    return ( ProxyInterface->AddRef() );
}

unsigned long m_IDirect3D9Ex::Release( void ) {
    unsigned long count = ProxyInterface->Release();

    if ( count == 0 ) {
        delete this;
    }

    return ( count );
}

HRESULT m_IDirect3D9Ex::EnumAdapterModes( THIS_ unsigned int Adapter,
                                          D3DFORMAT Format,
                                          unsigned int Mode,
                                          D3DDISPLAYMODE* pMode ) {
    return ( ProxyInterface->EnumAdapterModes( Adapter, Format, Mode, pMode ) );
}

unsigned int m_IDirect3D9Ex::GetAdapterCount( void ) {
    return ( ProxyInterface->GetAdapterCount() );
}

HRESULT m_IDirect3D9Ex::GetAdapterDisplayMode( unsigned int Adapter,
                                               D3DDISPLAYMODE* pMode ) {
    return ( ProxyInterface->GetAdapterDisplayMode( Adapter, pMode ) );
}

HRESULT m_IDirect3D9Ex::GetAdapterIdentifier(
    unsigned int Adapter,
    unsigned long Flags,
    D3DADAPTER_IDENTIFIER9* pIdentifier ) {
    return (
        ProxyInterface->GetAdapterIdentifier( Adapter, Flags, pIdentifier ) );
}

unsigned int m_IDirect3D9Ex::GetAdapterModeCount( THIS_ unsigned int Adapter,
                                                  D3DFORMAT Format ) {
    return ( ProxyInterface->GetAdapterModeCount( Adapter, Format ) );
}

HMONITOR m_IDirect3D9Ex::GetAdapterMonitor( unsigned int Adapter ) {
    return ( ProxyInterface->GetAdapterMonitor( Adapter ) );
}

HRESULT m_IDirect3D9Ex::GetDeviceCaps( unsigned int Adapter,
                                       D3DDEVTYPE DeviceType,
                                       D3DCAPS9* pCaps ) {
    return ( ProxyInterface->GetDeviceCaps( Adapter, DeviceType, pCaps ) );
}

HRESULT m_IDirect3D9Ex::RegisterSoftwareDevice( void* pInitializeFunction ) {
    return ( ProxyInterface->RegisterSoftwareDevice( pInitializeFunction ) );
}

HRESULT m_IDirect3D9Ex::CheckDepthStencilMatch( unsigned int Adapter,
                                                D3DDEVTYPE DeviceType,
                                                D3DFORMAT AdapterFormat,
                                                D3DFORMAT RenderTargetFormat,
                                                D3DFORMAT DepthStencilFormat ) {
    return ( ProxyInterface->CheckDepthStencilMatch(
        Adapter, DeviceType, AdapterFormat, RenderTargetFormat,
        DepthStencilFormat ) );
}

HRESULT m_IDirect3D9Ex::CheckDeviceFormat( unsigned int Adapter,
                                           D3DDEVTYPE DeviceType,
                                           D3DFORMAT AdapterFormat,
                                           unsigned long Usage,
                                           D3DRESOURCETYPE RType,
                                           D3DFORMAT CheckFormat ) {
    return ( ProxyInterface->CheckDeviceFormat(
        Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat ) );
}

HRESULT m_IDirect3D9Ex::CheckDeviceMultiSampleType(
    THIS_ unsigned int Adapter,
    D3DDEVTYPE DeviceType,
    D3DFORMAT SurfaceFormat,
    int Windowed,
    D3DMULTISAMPLE_TYPE MultiSampleType,
    unsigned long* pQualityLevels ) {
    return ( ProxyInterface->CheckDeviceMultiSampleType(
        Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType,
        pQualityLevels ) );
}

HRESULT m_IDirect3D9Ex::CheckDeviceType( unsigned int Adapter,
                                         D3DDEVTYPE CheckType,
                                         D3DFORMAT DisplayFormat,
                                         D3DFORMAT BackBufferFormat,
                                         int Windowed ) {
    return ( ProxyInterface->CheckDeviceType( Adapter, CheckType, DisplayFormat,
                                              BackBufferFormat, Windowed ) );
}

HRESULT m_IDirect3D9Ex::CheckDeviceFormatConversion( THIS_ unsigned int Adapter,
                                                     D3DDEVTYPE DeviceType,
                                                     D3DFORMAT SourceFormat,
                                                     D3DFORMAT TargetFormat ) {
    return ( ProxyInterface->CheckDeviceFormatConversion(
        Adapter, DeviceType, SourceFormat, TargetFormat ) );
}

// HRESULT m_IDirect3D9Ex::CreateDevice(unsigned int Adapter, D3DDEVTYPE
// DeviceType, HWND hFocusWindow, unsigned long BehaviorFlags, D3DPRESENT_PARAMETERS
// *pPresentationParameters, IDirect3DDevice9 **ppReturnedDeviceInterface)
//{
//	HRESULT hr = ProxyInterface->CreateDevice(Adapter, DeviceType,
// hFocusWindow, BehaviorFlags, pPresentationParameters,
// ppReturnedDeviceInterface);
//
//	if (SUCCEEDED(hr) && ppReturnedDeviceInterface)
//	{
//		*ppReturnedDeviceInterface = new
// m_IDirect3DDevice9Ex((IDirect3DDevice9Ex*)*ppReturnedDeviceInterface, this,
// IID_IDirect3DDevice9);
//	}
//
//	return hr;
// }

unsigned int m_IDirect3D9Ex::GetAdapterModeCountEx(
    THIS_ unsigned int Adapter,
    const D3DDISPLAYMODEFILTER* pFilter ) {
    return ( ProxyInterface->GetAdapterModeCountEx( Adapter, pFilter ) );
}

HRESULT m_IDirect3D9Ex::EnumAdapterModesEx( THIS_ unsigned int Adapter,
                                            const D3DDISPLAYMODEFILTER* pFilter,
                                            unsigned int Mode,
                                            D3DDISPLAYMODEEX* pMode ) {
    return (
        ProxyInterface->EnumAdapterModesEx( Adapter, pFilter, Mode, pMode ) );
}

HRESULT m_IDirect3D9Ex::GetAdapterDisplayModeEx(
    THIS_ unsigned int Adapter,
    D3DDISPLAYMODEEX* pMode,
    D3DDISPLAYROTATION* pRotation ) {
    return (
        ProxyInterface->GetAdapterDisplayModeEx( Adapter, pMode, pRotation ) );
}

// HRESULT m_IDirect3D9Ex::CreateDeviceEx(THIS_ unsigned int Adapter, D3DDEVTYPE
// DeviceType, HWND hFocusWindow, unsigned long BehaviorFlags, D3DPRESENT_PARAMETERS*
// pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode,
// IDirect3DDevice9Ex** ppReturnedDeviceInterface)
//{
//	HRESULT hr = ProxyInterface->CreateDeviceEx(Adapter, DeviceType,
// hFocusWindow, BehaviorFlags, pPresentationParameters, pFullscreenDisplayMode,
// ppReturnedDeviceInterface);
//
//	if (SUCCEEDED(hr) && ppReturnedDeviceInterface)
//	{
//		*ppReturnedDeviceInterface = new
// m_IDirect3DDevice9Ex(*ppReturnedDeviceInterface, this,
// IID_IDirect3DDevice9Ex);
//	}
//
//	return hr;
// }

HRESULT m_IDirect3D9Ex::GetAdapterLUID( THIS_ unsigned int Adapter,
                                        LUID* pLUID ) {
    return ( ProxyInterface->GetAdapterLUID( Adapter, pLUID ) );
}
