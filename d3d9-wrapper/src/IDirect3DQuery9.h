#pragma once

class m_IDirect3DQuery9 : public IDirect3DQuery9,
                          public AddressLookupTableObject {
private:
    LPDIRECT3DQUERY9 ProxyInterface;
    m_IDirect3DDevice9Ex* m_pDeviceEx = nullptr;

public:
    m_IDirect3DQuery9( LPDIRECT3DQUERY9 pQuery9, m_IDirect3DDevice9Ex* pDevice )
        : ProxyInterface( pQuery9 ), m_pDeviceEx( pDevice ) {
        pDevice->ProxyAddressLookupTable->SaveAddress( this, ProxyInterface );
    }
    ~m_IDirect3DQuery9() {}

    LPDIRECT3DQUERY9 GetProxyInterface( void ) { return ProxyInterface; }

    /*** IUnknown methods ***/
    STDMETHOD( QueryInterface )( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_( unsigned long, AddRef )( THIS );
    STDMETHOD_( unsigned long, Release )( THIS );

    /*** IDirect3DQuery9 methods ***/
    STDMETHOD( GetDevice )( THIS_ IDirect3DDevice9** ppDevice );
    STDMETHOD_( D3DQUERYTYPE, GetType )( THIS );
    STDMETHOD_( unsigned long, GetDataSize )( THIS );
    STDMETHOD( Issue )( THIS_ unsigned long dwIssueFlags );
    STDMETHOD( GetData )
    ( THIS_ void* pData, unsigned long dwSize, unsigned long dwGetDataFlags );
};
