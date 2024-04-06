#pragma once

#if __has_include( <imgui.h> )

#include <imgui.h>

#endif

// Disable some of MSVC most aggressive Debug runtime checks in function
// header/footer (used in some simple/low-level functions)
#if defined( _MSC_VER ) && !defined( __clang__ ) && \
    !defined( __INTEL_COMPILER ) && !defined( IMGUI_DEBUG_PARANOID )

#define IM_MSVC_RUNTIME_CHECKS_OFF                                       \
    __pragma( runtime_checks( "", off ) ) __pragma( check_stack( off ) ) \
        __pragma( strict_gs_check( push, off ) )

#define IM_MSVC_RUNTIME_CHECKS_RESTORE                                  \
    __pragma( runtime_checks( "", restore ) ) __pragma( check_stack() ) \
        __pragma( strict_gs_check( pop ) )

#else

#define IM_MSVC_RUNTIME_CHECKS_OFF
#define IM_MSVC_RUNTIME_CHECKS_RESTORE

#endif

enum ImGuiChildFlags_ {
    ImGuiChildFlags_None = 0,
    ImGuiChildFlags_Border =
        1 << 0, // Show an outer border and enable WindowPadding. (Important:
                // this is always == 1 == true for legacy reason)
    ImGuiChildFlags_AlwaysUseWindowPadding =
        1 << 1, // Pad with style.WindowPadding even if no border are drawn (no
                // padding by default for non-bordered child windows because it
                // makes more sense)
    ImGuiChildFlags_ResizeX =
        1 << 2, // Allow resize from right border (layout direction). Enable
                // .ini saving (unless ImGuiWindowFlags_NoSavedSettings passed
                // to window flags)
    ImGuiChildFlags_ResizeY =
        1 << 3, // Allow resize from bottom border (layout direction). "
    ImGuiChildFlags_AutoResizeX =
        1 << 4, // Enable auto-resizing width. Read "IMPORTANT: Size
                // measurement" details above.
    ImGuiChildFlags_AutoResizeY =
        1 << 5, // Enable auto-resizing height. Read "IMPORTANT: Size
                // measurement" details above.
    ImGuiChildFlags_AlwaysAutoResize =
        1 << 6, // Combined with AutoResizeX/AutoResizeY. Always measure size
                // even when child is hidden, always return true, always disable
                // clipping optimization! NOT RECOMMENDED.
    ImGuiChildFlags_FrameStyle =
        1 << 7, // Style the child window like a framed item: use FrameBg,
                // FrameRounding, FrameBorderSize, FramePadding instead of
                // ChildBg, ChildRounding, ChildBorderSize, WindowPadding.
};

IM_MSVC_RUNTIME_CHECKS_OFF
struct ImVec2 {
    float x, y;

    constexpr ImVec2() : x( 0.0f ), y( 0.0f ) {}
    constexpr ImVec2( float _x, float _y ) : x( _x ), y( _y ) {}

    float& operator[]( size_t idx ) {
        return ( ( float* )( void* )( char* )this )[ idx ];
    }

    float operator[]( size_t idx ) const {
        return ( ( const float* )( const void* )( const char* )this )[ idx ];
    }
};

typedef unsigned int ImGuiID; // A unique ID used by widgets (typically the
                              // result of hashing a stack of string)
typedef int
    ImGuiChildFlags; // -> enum ImGuiChildFlags_      // Flags: for BeginChild()
typedef int ImGuiSelectableFlags; // -> enum ImGuiSelectableFlags_ // Flags: for
                                  // Selectable()
typedef int ImGuiWindowFlags;     // -> enum ImGuiWindowFlags_     // Flags: for
                                  // Begin(), BeginChild()

typedef void( __cdecl* ImGui_Text_t )( const char* _formatString, ... );
