#pragma once

class m_IDirect3DSurface9 : public IDirect3DSurface9,
                            public AddressLookupTableObject {
private:
    LPDIRECT3DSURFACE9 ProxyInterface;
    m_IDirect3DDevice9Ex* m_pDeviceEx = nullptr;

public:
    m_IDirect3DSurface9( LPDIRECT3DSURFACE9 pSurface9,
                         m_IDirect3DDevice9Ex* pDevice )
        : ProxyInterface( pSurface9 ), m_pDeviceEx( pDevice ) {
        pDevice->ProxyAddressLookupTable->SaveAddress( this, ProxyInterface );
    }
    ~m_IDirect3DSurface9() {}

    LPDIRECT3DSURFACE9 GetProxyInterface( void ) { return ProxyInterface; }

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
    STDMETHOD( GetContainer )( THIS_ REFIID riid, void** ppContainer );
    STDMETHOD( GetDesc )( THIS_ D3DSURFACE_DESC* pDesc );
    STDMETHOD( LockRect )
    ( THIS_ D3DLOCKED_RECT* pLockedRect,
      CONST RECT* pRect,
      unsigned long Flags );
    STDMETHOD( UnlockRect )( THIS );
    STDMETHOD( GetDC )( THIS_ HDC* phdc );
    STDMETHOD( ReleaseDC )( THIS_ HDC hdc );
};
