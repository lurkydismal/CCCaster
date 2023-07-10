#pragma once

#include "TimerManager.hpp"

#include <unordered_map>

#define DEFAULT_HELD_DURATION ( 300 )

#define DEFAULT_REPEAT_INTERVAL ( 3 )

class KeyboardState {
   public:
    static void* windowHandle;

    static void clear();

    static void update();

    static bool isDown( uint32_t vkCode );

    static bool wasDown( uint32_t vkCode ) {
        const auto it = _previous.find( vkCode );

        if ( it == _previous.end() )
            return false;

        return it->second;
    }

    static inline bool isPressed( uint32_t vkCode ) {
        return ( isDown( vkCode ) && !wasDown( vkCode ) );
    }

    static inline bool isHeld(
        uint32_t vkCode,
        uint64_t duration = DEFAULT_HELD_DURATION,
        uint32_t repeatInterval = DEFAULT_REPEAT_INTERVAL ) {
        if ( !isDown( vkCode ) )
            return false;

        const auto it = _pressedTimestamp.find( vkCode );

        if ( it == _pressedTimestamp.end() )
            return false;

        const uint64_t now = TimerManager::get().getNow( true );

        return ( now - it->second >= duration ) &&
               ( _repeatTimer % repeatInterval == 0 );
    }

    static inline bool isPressedOrHeld(
        uint32_t vkCode,
        uint64_t duration = DEFAULT_HELD_DURATION,
        uint32_t repeatInterval = DEFAULT_REPEAT_INTERVAL ) {
        return ( isPressed( vkCode ) ||
                 isHeld( vkCode, duration, repeatInterval ) );
    }

    static inline bool isReleased( uint32_t vkCode ) {
        return ( !isDown( vkCode ) && wasDown( vkCode ) );
    }

    friend class KeyboardManager;

   private:
    static std::unordered_map< uint32_t, bool > _state, _previous;

    static std::unordered_map< uint32_t, uint64_t > _pressedTimestamp;

    static uint32_t _repeatTimer;
};
