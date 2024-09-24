#pragma once

class m_IDirect3DVolumeTexture9 : public IDirect3DVolumeTexture9,
                                  public AddressLookupTableObject {
private:
    LPDIRECT3DVOLUMETEXTURE9 ProxyInterface;
    m_IDirect3DDevice9Ex* m_pDeviceEx = nullptr;

public:
    m_IDirect3DVolumeTexture9( LPDIRECT3DVOLUMETEXTURE9 pTexture8,
                               m_IDirect3DDevice9Ex* pDevice )
        : ProxyInterface( pTexture8 ), m_pDeviceEx( pDevice ) {
        pDevice->ProxyAddressLookupTable->SaveAddress( this, ProxyInterface );
    }
    ~m_IDirect3DVolumeTexture9() {}

    LPDIRECT3DVOLUMETEXTURE9 GetProxyInterface( void ) {
        return ProxyInterface;
    }

    /*** IUnknown methods ***/
    STDMETHOD( QueryInterface )( THIS_ REFIID riid, void** ppvObj );
    STDMETHOD_( unsigned long, AddRef )( THIS );
    STDMETHOD_( unsigned long, Release )( THIS );

    /*** IDirect3DBaseTexture9 methods ***/
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
    STDMETHOD_( unsigned long, SetLOD )( THIS_ unsigned long LODNew );
    STDMETHOD_( unsigned long, GetLOD )( THIS );
    STDMETHOD_( unsigned long, GetLevelCount )( THIS );
    STDMETHOD( SetAutoGenFilterType )( THIS_ D3DTEXTUREFILTERTYPE FilterType );
    STDMETHOD_( D3DTEXTUREFILTERTYPE, GetAutoGenFilterType )( THIS );
    STDMETHOD_( void, GenerateMipSubLevels )( THIS );
    STDMETHOD( GetLevelDesc )
    ( THIS_ unsigned int Level, D3DVOLUME_DESC* pDesc );
    STDMETHOD( GetVolumeLevel )
    ( THIS_ unsigned int Level, IDirect3DVolume9** ppVolumeLevel );
    STDMETHOD( LockBox )
    ( THIS_ unsigned int Level,
      D3DLOCKED_BOX* pLockedVolume,
      CONST D3DBOX* pBox,
      unsigned long Flags );
    STDMETHOD( UnlockBox )( THIS_ unsigned int Level );
    STDMETHOD( AddDirtyBox )( THIS_ CONST D3DBOX* pDirtyBox );
};
