#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#undef min
#undef max

#include <omp.h>

#include <algorithm>
#include <map>
#include <type_traits>

#define MAX_INDEX 16

class AddressLookupTableObject {
public:
    virtual ~AddressLookupTableObject() {}

    void DeleteMe( void ) { delete this; }
};

template < typename D >
class AddressLookupTable {
public:
    explicit AddressLookupTable( D* pDevice ) : pDevice( pDevice ) {}
    ~AddressLookupTable() {
        ConstructorFlag = true;

#pragma omp simd
        for ( const auto& cache : g_map ) {
            for ( const auto& entry : cache ) {
                entry.second->DeleteMe();
            }
        }
    }

    template < class T, class Enable = void >
    struct AddressCacheIndex {
        static constexpr unsigned int CacheIndex = 0;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3D9Ex >::value >::type > {
        static constexpr unsigned int CacheIndex = 1;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DDevice9Ex >::value >::type > {
        static constexpr unsigned int CacheIndex = 2;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DCubeTexture9 >::value >::type > {
        static constexpr unsigned int CacheIndex = 3;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DIndexBuffer9 >::value >::type > {
        static constexpr unsigned int CacheIndex = 4;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DPixelShader9 >::value >::type > {
        static constexpr unsigned int CacheIndex = 5;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DQuery9 >::value >::type > {
        static constexpr unsigned int CacheIndex = 6;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DStateBlock9 >::value >::type > {
        static constexpr unsigned int CacheIndex = 7;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DSurface9 >::value >::type > {
        static constexpr unsigned int CacheIndex = 8;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DSwapChain9Ex >::value >::type > {
        static constexpr unsigned int CacheIndex = 9;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DTexture9 >::value >::type > {
        static constexpr unsigned int CacheIndex = 10;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DVertexBuffer9 >::value >::type > {
        static constexpr unsigned int CacheIndex = 11;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DVertexDeclaration9 >::value >::type > {
        static constexpr unsigned int CacheIndex = 12;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DVertexShader9 >::value >::type > {
        static constexpr unsigned int CacheIndex = 13;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DVolume9 >::value >::type > {
        static constexpr unsigned int CacheIndex = 14;
    };

    template < class T >
    struct AddressCacheIndex<
        T,
        typename std::enable_if<
            std::is_same< T, m_IDirect3DVolumeTexture9 >::value >::type > {
        static constexpr unsigned int CacheIndex = 15;
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

        constexpr unsigned int CacheIndex = AddressCacheIndex< T >::CacheIndex;

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
        constexpr unsigned int CacheIndex = AddressCacheIndex< T >::CacheIndex;

        if ( Wrapper && Proxy ) {
            g_map[ CacheIndex ][ Proxy ] = Wrapper;
        }
    }

    template < typename T >
    void DeleteAddress( T* Wrapper ) {
        if ( !Wrapper || ConstructorFlag ) {
            return;
        }

        constexpr unsigned int CacheIndex = AddressCacheIndex< T >::CacheIndex;

        auto it = std::find_if( g_map[ CacheIndex ].begin(),
                                g_map[ CacheIndex ].end(),
                                [ = ]( std::map< void*, T > Map ) -> int {
                                    return ( Map.second == Wrapper );
                                } );

        if ( it != std::end( g_map[ CacheIndex ] ) ) {
            it = g_map[ CacheIndex ].erase( it );
        }
    }

private:
    int ConstructorFlag = false;
    D* const pDevice;
    std::map< void*, AddressLookupTableObject* > g_map[ MAX_INDEX ];
};
