#include "d3d9.h"

long m_IDirect3DVertexDeclaration9::QueryInterface( THIS_ REFIID riid,
                                                    void** ppvObj ) {
    if ( ( riid == IID_IDirect3DVertexDeclaration9 || riid == IID_IUnknown ) &&
         ppvObj ) {
        AddRef();

        *ppvObj = this;

        return S_OK;
    }

    long hr = ProxyInterface->QueryInterface( riid, ppvObj );

    if ( SUCCEEDED( hr ) ) {
        genericQueryInterface( riid, ppvObj, m_pDeviceEx );
    }

    return hr;
}

unsigned long m_IDirect3DVertexDeclaration9::AddRef( THIS ) {
    return ProxyInterface->AddRef();
}

unsigned long m_IDirect3DVertexDeclaration9::Release( THIS ) {
    return ProxyInterface->Release();
}

long m_IDirect3DVertexDeclaration9::GetDevice(
    THIS_ IDirect3DDevice9** ppDevice ) {
    if ( !ppDevice ) {
        return D3DERR_INVALIDCALL;
    }

    m_pDeviceEx->AddRef();

    *ppDevice = m_pDeviceEx;

    return D3D_OK;
}

long m_IDirect3DVertexDeclaration9::GetDeclaration(
    THIS_ D3DVERTEXELEMENT9* pElement,
    unsigned int* pNumElements ) {
    return ProxyInterface->GetDeclaration( pElement, pNumElements );
}
