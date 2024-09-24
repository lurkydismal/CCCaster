#pragma once

class m_IDirect3DPixelShader9 : public IDirect3DPixelShader9,
                                public AddressLookupTableObject {
private:
    LPDIRECT3DPIXELSHADER9 ProxyInterface;
    m_IDirect3DDevice9Ex* m_pDeviceEx = nullptr;

public:
    m_IDirect3DPixelShader9( LPDIRECT3DPIXELSHADER9 pShader9,
                             m_IDirect3DDevice9Ex* pDevice )
        : ProxyInterface( pShader9 ), m_pDeviceEx( pDevice ) {
        pDevice->ProxyAddressLookupTable->SaveAddress( this, ProxyInterface );
    }
    ~m_IDirect3DPixelShader9() {}

    LPDIRECT3DPIXELSHADER9 GetProxyInterface( void ) { return ProxyInterface; }

    /*** IUnknown methods ***/
    STDMETHOD( QueryInterface )( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_( unsigned long, AddRef )( THIS );
    STDMETHOD_( unsigned long, Release )( THIS );

    /*** IDirect3DPixelShader9 methods ***/
    STDMETHOD( GetDevice )( THIS_ IDirect3DDevice9** ppDevice );
    STDMETHOD( GetFunction )( THIS_ void* pData, unsigned int* pSizeOfData );
};
