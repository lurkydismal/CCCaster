#include "d3d9.h"

HRESULT m_IDirect3DCubeTexture9::QueryInterface( THIS_ REFIID riid,
                                                 void** ppvObj ) {
    if ( ( riid == IID_IDirect3DCubeTexture9 || riid == IID_IUnknown ||
           riid == IID_IDirect3DResource9 ||
           riid == IID_IDirect3DBaseTexture9 ) &&
         ppvObj ) {
        AddRef();

        *ppvObj = this;

        return ( S_OK );
    }

    HRESULT hr = ProxyInterface->QueryInterface( riid, ppvObj );

    if ( SUCCEEDED( hr ) ) {
        genericQueryInterface( riid, ppvObj, m_pDeviceEx );
    }

    return ( hr );
}

unsigned long m_IDirect3DCubeTexture9::AddRef( THIS ) {
    return ( ProxyInterface->AddRef() );
}

unsigned long m_IDirect3DCubeTexture9::Release( THIS ) {
    return ( ProxyInterface->Release() );
}

HRESULT m_IDirect3DCubeTexture9::GetDevice(
    THIS_ IDirect3DDevice9** ppDevice ) {
    if ( !ppDevice ) {
        return ( D3DERR_INVALIDCALL );
    }

    m_pDeviceEx->AddRef();

    *ppDevice = m_pDeviceEx;

    return ( D3D_OK );
}

HRESULT m_IDirect3DCubeTexture9::SetPrivateData( THIS_ REFGUID refguid,
                                                 CONST void* pData,
                                                 unsigned long SizeOfData,
                                                 unsigned long Flags ) {
    return (
        ProxyInterface->SetPrivateData( refguid, pData, SizeOfData, Flags ) );
}

HRESULT m_IDirect3DCubeTexture9::GetPrivateData( THIS_ REFGUID refguid,
                                                 void* pData,
                                                 unsigned long* pSizeOfData ) {
    return ( ProxyInterface->GetPrivateData( refguid, pData, pSizeOfData ) );
}

HRESULT m_IDirect3DCubeTexture9::FreePrivateData( THIS_ REFGUID refguid ) {
    return ( ProxyInterface->FreePrivateData( refguid ) );
}

unsigned long m_IDirect3DCubeTexture9::SetPriority(
    THIS_ unsigned long PriorityNew ) {
    return ( ProxyInterface->SetPriority( PriorityNew ) );
}

unsigned long m_IDirect3DCubeTexture9::GetPriority( THIS ) {
    return ( ProxyInterface->GetPriority() );
}

void m_IDirect3DCubeTexture9::PreLoad( THIS ) {
    ProxyInterface->PreLoad();
}

D3DRESOURCETYPE m_IDirect3DCubeTexture9::GetType( THIS ) {
    return ( ProxyInterface->GetType() );
}

unsigned long m_IDirect3DCubeTexture9::SetLOD( THIS_ unsigned long LODNew ) {
    return ( ProxyInterface->SetLOD( LODNew ) );
}

unsigned long m_IDirect3DCubeTexture9::GetLOD( THIS ) {
    return ( ProxyInterface->GetLOD() );
}

unsigned long m_IDirect3DCubeTexture9::GetLevelCount( THIS ) {
    return ( ProxyInterface->GetLevelCount() );
}

HRESULT m_IDirect3DCubeTexture9::SetAutoGenFilterType(
    THIS_ D3DTEXTUREFILTERTYPE FilterType ) {
    return ( ProxyInterface->SetAutoGenFilterType( FilterType ) );
}

D3DTEXTUREFILTERTYPE m_IDirect3DCubeTexture9::GetAutoGenFilterType( THIS ) {
    return ( ProxyInterface->GetAutoGenFilterType() );
}

void m_IDirect3DCubeTexture9::GenerateMipSubLevels( THIS ) {
    return ( ProxyInterface->GenerateMipSubLevels() );
}

HRESULT m_IDirect3DCubeTexture9::GetLevelDesc( THIS_ unsigned int Level,
                                               D3DSURFACE_DESC* pDesc ) {
    return ( ProxyInterface->GetLevelDesc( Level, pDesc ) );
}

HRESULT m_IDirect3DCubeTexture9::GetCubeMapSurface(
    THIS_ D3DCUBEMAP_FACES FaceType,
    unsigned int Level,
    IDirect3DSurface9** ppCubeMapSurface ) {
    HRESULT hr =
        ProxyInterface->GetCubeMapSurface( FaceType, Level, ppCubeMapSurface );

    if ( ( SUCCEEDED( hr ) ) && ( ppCubeMapSurface ) ) {
        *ppCubeMapSurface =
            m_pDeviceEx->ProxyAddressLookupTable
                ->FindAddress< m_IDirect3DSurface9 >( *ppCubeMapSurface );
    }

    return ( hr );
}

HRESULT m_IDirect3DCubeTexture9::LockRect( THIS_ D3DCUBEMAP_FACES FaceType,
                                           unsigned int Level,
                                           D3DLOCKED_RECT* pLockedRect,
                                           CONST RECT* pRect,
                                           unsigned long Flags ) {
    return ( ProxyInterface->LockRect( FaceType, Level, pLockedRect, pRect,
                                       Flags ) );
}

HRESULT m_IDirect3DCubeTexture9::UnlockRect( THIS_ D3DCUBEMAP_FACES FaceType,
                                             unsigned int Level ) {
    return ( ProxyInterface->UnlockRect( FaceType, Level ) );
}

HRESULT m_IDirect3DCubeTexture9::AddDirtyRect( THIS_ D3DCUBEMAP_FACES FaceType,
                                               CONST RECT* pDirtyRect ) {
    return ( ProxyInterface->AddDirtyRect( FaceType, pDirtyRect ) );
}
