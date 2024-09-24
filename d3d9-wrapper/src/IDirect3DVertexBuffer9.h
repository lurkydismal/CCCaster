#pragma once

class m_IDirect3DVertexBuffer9 : public IDirect3DVertexBuffer9,
                                 public AddressLookupTableObject {
private:
    LPDIRECT3DVERTEXBUFFER9 ProxyInterface;
    m_IDirect3DDevice9Ex* m_pDeviceEx = nullptr;

public:
    m_IDirect3DVertexBuffer9( LPDIRECT3DVERTEXBUFFER9 pBuffer8,
                              m_IDirect3DDevice9Ex* pDevice )
        : ProxyInterface( pBuffer8 ), m_pDeviceEx( pDevice ) {
        pDevice->ProxyAddressLookupTable->SaveAddress( this, ProxyInterface );
    }
    ~m_IDirect3DVertexBuffer9() {}

    LPDIRECT3DVERTEXBUFFER9 GetProxyInterface( void ) { return ProxyInterface; }

    /*** IUnknown methods ***/
    STDMETHOD( QueryInterface )( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_( unsigned long, AddRef )( THIS );
    STDMETHOD_( unsigned long, Release )( THIS );

    /*** IDirect3DResource9 methods ***/
    STDMETHOD( GetDevice )( THIS_ IDirect3DDevice9** ppDevice );
    STDMETHOD( SetPrivateData )
    ( THIS_ REFGUID refguid,
      CONST void* pData,
      unsigned long SizeOfData,
      unsigned long Flags );
    STDMETHOD( GetPrivateData )
    ( THIS_ REFGUID refguid, void* pData, unsigned long* pSizeOfData );
    STDMETHOD( FreePrivateData )( THIS_ REFGUID refguid );
    STDMETHOD_( unsigned long, SetPriority )( THIS_ unsigned long PriorityNew );
    STDMETHOD_( unsigned long, GetPriority )( THIS );
    STDMETHOD_( void, PreLoad )( THIS );
    STDMETHOD_( D3DRESOURCETYPE, GetType )( THIS );
    STDMETHOD( Lock )
    ( THIS_ unsigned int OffsetToLock,
      unsigned int SizeToLock,
      void** ppbData,
      unsigned long Flags );
    STDMETHOD( Unlock )( THIS );
    STDMETHOD( GetDesc )( THIS_ D3DVERTEXBUFFER_DESC* pDesc );
};
