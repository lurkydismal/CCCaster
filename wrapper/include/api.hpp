#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>

namespace wrapper {

[[nodiscard]] auto makePatch( uintptr_t _address,
                              std::span< const std::byte > _bytes ) -> size_t;
[[nodiscard]] auto makePatchByPattern( uintptr_t _address,
                                       std::string_view _pattern,
                                       std::span< const std::byte > _bytes )
    -> std::expected< size_t, std::string >;
void removePatch( size_t _id );

using patch_t = struct patch {
    patch( uintptr_t _address,
           std::span< const std::byte > _bytes,
           decltype( &wrapper::makePatch ) _makePatch,
           decltype( &wrapper::removePatch ) _removePatch )
        : patch( _makePatch( _address, _bytes ), _removePatch ) {}

    patch( uintptr_t _address,
           std::string_view _pattern,
           std::span< const std::byte > _bytes,
           decltype( &wrapper::makePatchByPattern ) _makePatchByPattern,
           decltype( &wrapper::removePatch ) _removePatch )
        : _removePatch( _removePatch ) {
        const auto l_patches =
            _makePatchByPattern( _address, _pattern, _bytes );

        if ( l_patches ) {
            _id = l_patches.value();
        }
    }

    ~patch() { _removePatch( _id ); }

    patch( const patch& ) = delete;
    patch( patch&& ) = default;
    auto operator=( const patch& ) -> patch& = delete;
    auto operator=( patch&& ) -> patch& = default;

private:
    patch( size_t _id, decltype( &wrapper::removePatch ) _removePatch )
        : _id( _id ), _removePatch( _removePatch ) {}

private:
    size_t _id;
    decltype( &wrapper::removePatch ) _removePatch;
};

using api_t = struct api {
    api( decltype( &makePatch ) _makePatch,
         decltype( &makePatchByPattern ) _makePatchByPattern,
         decltype( &removePatch ) _removePatch )
        : _makePatch( _makePatch ),
          _makePatchByPattern( _makePatchByPattern ),
          _removePatch( _removePatch ) {}

    ~api() = default;

    api( const api& ) = default;
    api( api&& ) = default;
    auto operator=( const api& ) -> api& = default;
    auto operator=( api&& ) -> api& = default;

    [[nodiscard]] auto makePatch( uintptr_t _address,
                                  std::span< const std::byte > _bytes )
        -> patch_t {
        return { _address, _bytes, _makePatch, _removePatch };
    }

    [[nodiscard]] auto makePatch( uintptr_t _address,
                                  std::string_view _pattern,
                                  std::span< const std::byte > _bytes )
        -> patch_t {
        return { _address, _pattern, _bytes, _makePatchByPattern,
                 _removePatch };
    }

private:
    decltype( &wrapper::makePatch ) _makePatch;
    decltype( &wrapper::makePatchByPattern ) _makePatchByPattern;
    decltype( &wrapper::removePatch ) _removePatch;
};

} // namespace wrapper