#include <cstdlib>
#include <print>
#include <span>

#include "addon_loader.hpp"
#include "api.hpp"
#include "example.hpp"
#include "logg.hpp"
#include "stddebug.hpp"
#include "stdfunc.hpp"
#include "stdhash.hpp"
#include "store.hpp"
// #include "watch.hpp"

#if defined( __SANITIZE_LEAK__ )

#include <sanitizer/lsan_interface.h>

#endif

template < typename K, typename V, typename Cmp, typename Alloc >
struct std::formatter< std::unordered_map< K, V, Cmp, Alloc > > {
    constexpr auto parse( std::format_parse_context& _ctx ) {
        return _ctx.begin();
    }

    auto format( const std::unordered_map< K, V, Cmp, Alloc >& _m,
                 std::format_context& _ctx ) const {
        auto l_out = _ctx.out();
        *l_out++ = '{';

        for ( auto l_it = _m.begin(); l_it != _m.end(); ++l_it ) {
            l_out =
                std::format_to( l_out, "{}: {}", l_it->first, l_it->second );

            if ( std::next( l_it ) != _m.end() ) {
                l_out = std::format_to( l_out, ", " );
            }
        }

        *l_out++ = '}';

        return ( l_out );
    }
};

wrapper::api_t store::g_api;

[[gnu::visibility( "default" )]] auto init(
    decltype( &wrapper::makePatch ) _makePatch,
    decltype( &wrapper::makePatchByPattern ) _makePatchByPattern,
    decltype( &wrapper::removePatch ) _removePatch ) -> bool {
    static_assert(
        std::is_same_v< decltype( init ),
                        std::remove_pointer_t< wrapper::initFunction_t > > );

    example::printBuildType();

#if defined( __SANITIZE_LEAK__ )

    __lsan_do_leak_check();

#endif

    logg$error( "SUCCESS" );

    store::g_api =
        wrapper::api_t{ _makePatch, _makePatchByPattern, _removePatch };

    // LUA
    mod::AddonLoaderT l_loader{ "addons" };

    if ( !l_loader.scan() ) {
        std::cerr << "Scan failed\n";
        return 1;
    }

    if ( !l_loader.loadAll() ) {
        std::cerr << "Load failed\n";
        return 1;
    }

    std::println( "ADDONS: {{\n{}\n}}", l_loader.addons() );

    l_loader.dispatchEvent( "on_init" );

    // watch::watch_t l_watch{
    //     "addons",
    //     watch::watch_t::callbackDirectory_t{
    //         [ & ]( [[maybe_unused]] std::string_view _fileName,
    //                [[maybe_unused]] watch::event_t _event,
    //                [[maybe_unused]] uint32_t _cookie ) -> bool {
    //             l_loader.request_reload();
    //             return true;
    //         } },
    //     static_cast< watch::event_t >(
    //         static_cast< uint16_t >( watch::event_t::write ) |
    //         static_cast< uint16_t >( watch::event_t::rename ) |
    //         static_cast< uint16_t >( watch::event_t::remove ) ) };

    // while ( true ) {
    //     l_watch.check( true );
    //     l_loader.tick( false );
    // }

    return ( EXIT_FAILURE );
}
