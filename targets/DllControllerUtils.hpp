#pragma once

#include "Controller.hpp"
#include "ProcessManager.hpp"

struct DllControllerUtils {
    // Filter simultaneous up / down and left / right directions.
    // Prioritize down and left for keyboard only.
    static uint16_t filterSimulDirState( uint16_t state, uint32_t socdMode ) {
        if ( socdMode & 2 ) {
            if ( ( state & ( BIT_UP | BIT_DOWN ) ) == ( BIT_UP | BIT_DOWN ) )
                state &= ~( BIT_UP | BIT_DOWN );
        } else if ( socdMode & 4 ) {
            if ( ( state & ( BIT_UP | BIT_DOWN ) ) == ( BIT_UP | BIT_DOWN ) )
                state &= ~BIT_DOWN;
        } else {
            if ( ( state & ( BIT_UP | BIT_DOWN ) ) == ( BIT_UP | BIT_DOWN ) )
                state &= ~BIT_UP;
        }
        if ( socdMode & 1 ) {
            if ( ( state & ( BIT_LEFT | BIT_RIGHT ) ) ==
                 ( BIT_LEFT | BIT_RIGHT ) )
                state &= ~( BIT_LEFT | BIT_RIGHT );
        } else {
            if ( ( state & ( BIT_LEFT | BIT_RIGHT ) ) ==
                 ( BIT_LEFT | BIT_RIGHT ) )
                state &= ~BIT_RIGHT;
        }

        return state;
    }

    static uint16_t convertInputState( uint32_t state, uint32_t socdMode ) {
        const uint16_t dirs =
            filterSimulDirState( state & MASK_DIRS, socdMode );
        const uint16_t buttons = ( state & MASK_BUTTONS ) >> 8;

        uint8_t direction = 5;

        if ( dirs & BIT_UP )
            direction = 8;
        else if ( dirs & BIT_DOWN )
            direction = 2;

        if ( dirs & BIT_LEFT )
            --direction;
        else if ( dirs & BIT_RIGHT )
            ++direction;

        if ( direction == 5 )
            direction = 0;

        return COMBINE_INPUT( direction, buttons );
    }

    static uint16_t getPrevInput( const Controller* controller ) {
        if ( !controller )
            return 0;

        return convertInputState( controller->getPrevState(),
                                  controller->getSocdInt() );
    }

    static uint16_t getInput( const Controller* controller ) {
        if ( !controller )
            return 0;

        return convertInputState( controller->getState(),
                                  controller->getSocdInt() );
    }

    static bool isButtonPressed( const Controller* controller,
                                 uint16_t button ) {
        if ( !controller )
            return false;

        button = COMBINE_INPUT( 0, button );
        return ( getInput( controller ) & button ) &&
               !( getPrevInput( controller ) & button );
    }

    static bool isDirectionPressed( const Controller* controller,
                                    uint16_t dir ) {
        if ( !controller )
            return false;

        return ( ( getInput( controller ) & MASK_DIRS ) == dir ) &&
               ( ( getPrevInput( controller ) & MASK_DIRS ) != dir );
    }

    static uint8_t numJoystickButtonsDown( const Controller* controller ) {
        if ( !controller || !controller->isJoystick() )
            return false;

        const uint32_t buttons = controller->getJoystickState().buttons;

        uint8_t count = 0;

        for ( uint8_t i = 0; i < 16; ++i ) {
            if ( buttons & ( 1u << i ) )
                ++count;
        }

        return count;
    }
};
