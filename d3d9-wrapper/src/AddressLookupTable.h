#pragma once

#undef min
#undef max

#include <algorithm>
#include <type_traits>
#include <unordered_map>

constexpr UINT MaxIndex = 16;

template < typename D >
class AddressLookupTable {
public:
    explicit AddressLookupTable( D* pDevice ) : pDevice( pDevice ) {}
    ~AddressLookupTable() {
        ConstructorFlag = true;

        for ( const auto& cache : g_map ) {
            for ( const auto& entry : cache ) {
                entry.second->DeleteMe();
            }
        }
    }

    template < class T, class Enable = void >
    struct AddressCacheIndex {
        static constexpr UINT CacheIndex = 0;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3D9Ex >::value >::type > {
        static constexpr UINT CacheIndex = 1;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DDevice9Ex >::value >::type > {
        static constexpr UINT CacheIndex = 2;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DCubeTexture9 >::value >::type > {
        static constexpr UINT CacheIndex = 3;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DIndexBuffer9 >::value >::type > {
        static constexpr UINT CacheIndex = 4;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DPixelShader9 >::value >::type > {
        static constexpr UINT CacheIndex = 5;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DQuery9 >::value >::type > {
        static constexpr UINT CacheIndex = 6;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DStateBlock9 >::value >::type > {
        static constexpr UINT CacheIndex = 7;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DSurface9 >::value >::type > {
        static constexpr UINT CacheIndex = 8;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DSwapChain9Ex >::value >::type > {
        static constexpr UINT CacheIndex = 9;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DTexture9 >::value >::type > {
        static constexpr UINT CacheIndex = 10;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DVertexBuffer9 >::value >::type > {
        static constexpr UINT CacheIndex = 11;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DVertexDeclaration9 >::value >::type > {
        static constexpr UINT CacheIndex = 12;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DVertexShader9 >::value >::type > {
        static constexpr UINT CacheIndex = 13;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DVolume9 >::value >::type > {
        static constexpr UINT CacheIndex = 14;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DVolumeTexture9 >::value >::type > {
        static constexpr UINT CacheIndex = 15;
    };

    m_IDirect3DSwapChain9Ex* CreateInterface( void* Proxy, REFIID riid ) {
        return ( new m_IDirect3DSwapChain9Ex(
            static_cast< m_IDirect3DSwapChain9Ex* >( Proxy ), pDevice, riid ) );
    }

    template < typename T >
    T* CreateInterface( void* Proxy ) {
        return ( new T( static_cast< T* >( Proxy ), pDevice ) );
    }

    template < typename T >
    T* FindAddress( void* Proxy, REFIID riid = IID_IUnknown ) {
        if ( !Proxy ) {
            return ( nullptr );
        }

        constexpr UINT CacheIndex = AddressCacheIndex< T >::CacheIndex;

        auto it = g_map[ CacheIndex ].find( Proxy );

        if ( it != std::end( g_map[ CacheIndex ] ) ) {
            return ( static_cast< T* >( it->second ) );
        }

        if ( riid == IID_IUnknown ) {
            return ( CreateInterface< T >( Proxy ) );

        } else {
            return ( ( T* )CreateInterface( Proxy, riid ) );
        }
    }

    template < typename T >
    void SaveAddress( T* Wrapper, void* Proxy ) {
        constexpr UINT CacheIndex = AddressCacheIndex< T >::CacheIndex;

        if ( Wrapper && Proxy ) {
            g_map[ CacheIndex ][ Proxy ] = Wrapper;
        }
    }

    template < typename T >
    void DeleteAddress( T* Wrapper ) {
        if ( !Wrapper || ConstructorFlag ) {
            return;
        }

        constexpr UINT CacheIndex = AddressCacheIndex< T >::CacheIndex;

        auto it = std::find_if(
            g_map[ CacheIndex ].begin(), g_map[ CacheIndex ].end(),
            [ = ]( auto Map ) -> BOOL { return ( Map.second == Wrapper ); } );

        if ( it != std::end( g_map[ CacheIndex ] ) ) {
            it = g_map[ CacheIndex ].erase( it );
        }
    }

private:
    BOOL ConstructorFlag = false;
    D* const pDevice;
    std::unordered_map< void*, class AddressLookupTableObject* >
        g_map[ MaxIndex ];
};

class AddressLookupTableObject {
public:
    virtual ~AddressLookupTableObject() {}

    void DeleteMe( void ) { delete this; }
};