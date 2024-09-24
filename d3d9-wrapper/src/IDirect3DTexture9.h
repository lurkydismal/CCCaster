#pragma once

class m_IDirect3DTexture9 : public IDirect3DTexture9,
                            public AddressLookupTableObject {
private:
    LPDIRECT3DTEXTURE9 ProxyInterface;
    m_IDirect3DDevice9Ex* m_pDeviceEx = nullptr;

public:
    m_IDirect3DTexture9( LPDIRECT3DTEXTURE9 pTexture9,
                         m_IDirect3DDevice9Ex* pDevice )
        : ProxyInterface( pTexture9 ), m_pDeviceEx( pDevice ) {
        pDevice->ProxyAddressLookupTable->SaveAddress( this, ProxyInterface );
    }
    ~m_IDirect3DTexture9() {}

    LPDIRECT3DTEXTURE9 GetProxyInterface( void ) { return ProxyInterface; }

    /*** IUnknown methods ***/
    STDMETHOD( QueryInterface )( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_( unsigned long, AddRef )( THIS );
    STDMETHOD_( unsigned long, Release )( THIS );

    /*** IDirect3DBaseTexture9 methods ***/
    STDMETHOD( GetDevice )( THIS_ IDirect3DDevice9** ppDevice );
    STDMETHOD( SetPrivateData )
    ( THIS_ REFGUID refguid, CONST void* pData, unsigned long SizeOfData, unsigned long Flags );
    STDMETHOD( GetPrivateData )
    ( THIS_ REFGUID refguid, void* pData, unsigned long* pSizeOfData );
    STDMETHOD( FreePrivateData )( THIS_ REFGUID refguid );
    STDMETHOD_( unsigned long, SetPriority )( THIS_ unsigned long PriorityNew );
    STDMETHOD_( unsigned long, GetPriority )( THIS );
    STDMETHOD_( void, PreLoad )( THIS );
    STDMETHOD_( D3DRESOURCETYPE, GetType )( THIS );
    STDMETHOD_( unsigned long, SetLOD )( THIS_ unsigned long LODNew );
    STDMETHOD_( unsigned long, GetLOD )( THIS );
    STDMETHOD_( unsigned long, GetLevelCount )( THIS );
    STDMETHOD( SetAutoGenFilterType )( THIS_ D3DTEXTUREFILTERTYPE FilterType );
    STDMETHOD_( D3DTEXTUREFILTERTYPE, GetAutoGenFilterType )( THIS );
    STDMETHOD_( void, GenerateMipSubLevels )( THIS );
    STDMETHOD( GetLevelDesc )( THIS_ unsigned int Level, D3DSURFACE_DESC* pDesc );
    STDMETHOD( GetSurfaceLevel )
    ( THIS_ unsigned int Level, IDirect3DSurface9** ppSurfaceLevel );
    STDMETHOD( LockRect )
    ( THIS_ unsigned int Level,
      D3DLOCKED_RECT* pLockedRect,
      CONST RECT* pRect,
      unsigned long Flags );
    STDMETHOD( UnlockRect )( THIS_ unsigned int Level );
    STDMETHOD( AddDirtyRect )( THIS_ CONST RECT* pDirtyRect );
};
