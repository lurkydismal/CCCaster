#include "DllControllerManager.hpp"
#include "DllOverlayUi.hpp"
#include "DllHacks.hpp"
#include "DllAsmHacks.hpp"
#include "KeyboardState.hpp"
#include "CharacterSelect.hpp"

#include <windows.h>

using namespace std;


#define VK_TOGGLE_OVERLAY ( VK_F4 )
#define VK_ENABLE_FRAMESTEP ( VK_F5 )

extern bool stopping;


void DllControllerManager::initControllers ( const ControllerMappings& mappings )
{
    Lock lock ( ControllerManager::get().mutex );

    ControllerManager::get().getKeyboard()->setMappings ( ProcessManager::fetchKeyboardConfig() );
    ControllerManager::get().setMappings ( mappings );
    ControllerManager::get().check();

    _allControllers = ControllerManager::get().getControllers();

    // Set the owner (Which enables callbacks) AFTER we have the list of all controllers
    ControllerManager::get().owner = this;

    // The first controller is keyboard which is always attached.
    // So we only set this flag for controllers beyond the first one.
    _controllerAttached = ( _allControllers.size() > 1 );
}

bool DllControllerManager::isNotMapping() const
{
    Lock lock ( ControllerManager::get().mutex );

    return  ( !_playerControllers[0] || !_playerControllers[0]->isMapping() )
            && ( !_playerControllers[1] || !_playerControllers[1]->isMapping() );
}

void DllControllerManager::updateControls ( uint16_t *localInputs )
{
    if ( stopping )
        return;

    Lock lock ( ControllerManager::get().mutex );

    bool toggleOverlay = false;

    // // Automatically show overlay when a controller is attached during chara select
    // if ( _controllerAttached )
    // {
    //     _controllerAttached = false;

    //     if ( !DllOverlayUi::isEnabled() && *CC_GAME_MODE_ADDR == CC_GAME_MODE_CHARA_SELECT )
    //         toggleOverlay = true;
    // }

    // Toggle with a keyboard hotkey
    if ( KeyboardState::isPressed ( VK_TOGGLE_OVERLAY ) )
    {
        toggleOverlay = true;
    }
    if ( KeyboardState::isPressed ( VK_ENABLE_FRAMESTEP ) )
    {
        framestepEnabled = true;
    }

    for ( Controller *controller : _allControllers )
    {
        // Don't sticky controllers if the overlay is enabled
        if ( DllOverlayUi::isEnabled() )
            continue;

        // Sticky controllers to the first available player when anything is pressed
        if ( getInput ( controller ) )
        {
            if ( isSinglePlayer )
            {
                // Only sticky the local player in single player modes
                if ( !_playerControllers[localPlayer - 1] && controller != _playerControllers[localPlayer - 1] )
                {
                    _playerControllers[localPlayer - 1] = controller;
                }
            }
            else
            {
                // Sticky player 1 then player 2 optimistically
                if ( !_playerControllers[0] && controller != _playerControllers[1] )
                {
                    _playerControllers[0] = controller;
                }
                else if ( !_playerControllers[1] && controller != _playerControllers[0] )
                {
                    _playerControllers[1] = controller;
                }
            }
        }

        // Toggle with 3 held joystick buttons + any direction during chara select
        if ( *CC_GAME_MODE_ADDR == CC_GAME_MODE_CHARA_SELECT
                && numJoystickButtonsDown ( controller ) >= 3
                && !controller->getJoystickState().isNeutral() )
        {
            toggleOverlay = true;
        }
    }

    // Show message takes priority over the overlay UI
    if ( DllOverlayUi::isShowingMessage() )
    {
        DllOverlayUi::updateMessage();
    }
    else if ( toggleOverlay )
    {
        toggleOverlay = false;

        if ( ! DllOverlayUi::isEnabled() )
        {
            // Refresh the list of joysticks if we're enabling the overlay
            ControllerManager::get().refreshJoysticks();

            // Enable keyboard events, this effectively eats all keyboard inputs for the window
            KeyboardManager::get().keyboardWindow = DllHacks::windowHandle;
            KeyboardManager::get().matchedKeys.clear();
            KeyboardManager::get().ignoredKeys.clear();
            KeyboardManager::get().hook ( this, true );

            // Disable Escape to exit
            AsmHacks::enableEscapeToExit = false;

            // Enable overlay
            DllOverlayUi::setMapping();
            DllOverlayUi::enable();
        }
        else if ( DllOverlayUi::isMapping() )
        {
            // Cancel all mapping if we're disabling the overlay
            if ( _playerControllers[0] )
                _playerControllers[0]->cancelMapping();
            if ( _playerControllers[1] )
                _playerControllers[1]->cancelMapping();

            _overlayPositions[0] = 0;
            _overlayPositions[1] = 0;

            // Disable keyboard events, since we use GetKeyState for regular controller inputs
            KeyboardManager::get().unhook();

            // Re-enable Escape to exit
            AsmHacks::enableEscapeToExit = true;

            // Disable overlay
            DllOverlayUi::disable();
        }
    }

    // Only update player controls when the overlay is NOT enabled
    if ( !DllOverlayUi::isEnabled() )
    {
        if ( _playerControllers[localPlayer - 1] ) {
            uint16_t input = getInput ( _playerControllers[localPlayer - 1] );
            if ( localPlayer == 1 ) {
                if ( *CC_P1_FACING_FLAG_ADDR )
                    input |= COMBINE_INPUT ( 0, CC_PLAYER_FACING );
            }
            if ( localPlayer == 2) {
                if ( *CC_P2_FACING_FLAG_ADDR )
                    input |= COMBINE_INPUT ( 0, CC_PLAYER_FACING );
            }
            localInputs[0] = input;
        }

        if ( _playerControllers[remotePlayer - 1] ) {
            uint16_t input = getInput ( _playerControllers[remotePlayer - 1] );
            if ( remotePlayer == 1 ) {
                if ( *CC_P1_FACING_FLAG_ADDR )
                    input |= COMBINE_INPUT ( 0, CC_PLAYER_FACING );
            }
            if ( remotePlayer == 2) {
                if ( *CC_P2_FACING_FLAG_ADDR )
                    input |= COMBINE_INPUT ( 0, CC_PLAYER_FACING );
            }
            localInputs[1] = input;
        }
        return;
    }

    if ( DllOverlayUi::isMapping() )
    {
        handleMappingOverlay();
    }
}

void DllControllerManager::handleMappingOverlay()
{
    // Check all controllers
    for ( Controller *controller : _allControllers )
    {
        if ( ( controller->isJoystick() && isDirectionPressed ( controller, 6 ) )
                || ( controller->isKeyboard() && KeyboardState::isPressed ( VK_RIGHT ) ) )
        {
            // Move controller right
            if ( controller == _playerControllers[0] )
            {
                if ( ! ( controller->isKeyboard() && controller->isMapping()
                         && _overlayPositions[0] >= 1 && _overlayPositions[0] <= 4 ) )
                {
                    _playerControllers[0]->cancelMapping();
                    _playerControllers[0] = 0;
                }
            }
            else if ( isSinglePlayer && localPlayer == 1 )
            {
                // Only one controller (player 1)
                continue;
            }
            else if ( ! _playerControllers[1] )
            {
                _playerControllers[1] = controller;
                _overlayPositions[1] = 0;
            }
        }
        else if ( ( controller->isJoystick() && isDirectionPressed ( controller, 4 ) )
                  || ( controller->isKeyboard() && KeyboardState::isPressed ( VK_LEFT ) ) )
        {
            // Move controller left
            if ( controller == _playerControllers[1] )
            {
                if ( ! ( controller->isKeyboard() && controller->isMapping()
                         && _overlayPositions[1] >= 1 && _overlayPositions[1] <= 4 ) )
                {
                    _playerControllers[1]->cancelMapping();
                    _playerControllers[1] = 0;
                }
            }
            else if ( isSinglePlayer && localPlayer == 2 )
            {
                // Only one controller (player 2)
                continue;
            }
            else if ( ! _playerControllers[0] )
            {
                _playerControllers[0] = controller;
                _overlayPositions[0] = 0;
            }
        }
    }

    array<string, 3> text;

    // Display all controllers
    text[1] = "Controllers\n";
    for ( const Controller *controller : _allControllers )
        if ( controller != _playerControllers[0] && controller != _playerControllers[1] )
            text[1] += "\n" + controller->getName();

    const size_t controllersHeight = 3 + _allControllers.size();

    // Update player controllers
    for ( uint8_t i = 0; i < 2; ++i )
    {
        string& playerText = text [ i ? 2 : 0 ];

        // Hide / disable other player's overlay in netplay
        if ( isSinglePlayer && localPlayer != i + 1 )
        {
            playerText.clear();
            DllOverlayUi::updateSelector ( i );
            continue;
        }

        // Show placeholder when player has no controller assigned
        if ( ! _playerControllers[i] )
        {
            playerText = ( i == 0 ? "Press Left on P1 controller" : "Press Right on P2 controller" );
            DllOverlayUi::updateSelector ( i );
            continue;
        }

        // Generate mapping options starting with controller name
        size_t headerHeight = 0;
        vector<string> options;
        options.push_back ( _playerControllers[i]->getName() );

        if ( _playerControllers[i]->isKeyboard() )
        {
            headerHeight = max ( 3u, controllersHeight );

            // Instructions for mapping keyboard controls
            playerText = "Press Enter to set a direction key\n";
            playerText += format ( "Press %s to delete a key\n", ( i == 0 ? "Left" : "Right" ) );
            playerText += string ( headerHeight - 3, '\n' );

            // Add directions to keyboard mapping options
            for ( size_t j = 0; j < 4; ++j )
            {
                const string mapping = _playerControllers[i]->getMapping ( gameInputBits[j].second, "..." );
                options.push_back ( gameInputBits[j].first + " : " + mapping );
            }
        }
        else
        {
            headerHeight = max ( 2u, controllersHeight );

            // Instructions for mapping joystick buttons
            playerText = format ( "Press %s to delete a key\n", ( i == 0 ? "Left" : "Right" ) );
            playerText += string ( headerHeight - 2, '\n' );
        }

        // Add buttons to mapping options
        for ( size_t j = 4; j < gameInputBits.size(); ++j )
        {
            const string mapping = _playerControllers[i]->getMapping ( gameInputBits[j].second );
            options.push_back ( gameInputBits[j].first + " : " + mapping );
        }

        // Finally add done option
        options.push_back ( _playerControllers[i]->isKeyboard() ? "Done (press Enter)" : "Done (press any button)" );

        // Disable overlay if both players are done
        if ( _overlayPositions[i] + 1 == options.size()
                && ( ( _playerControllers[i]->isJoystick() && _playerControllers[i]->getJoystickState().buttons )
                     || ( _playerControllers[i]->isKeyboard() && KeyboardState::isPressed ( VK_RETURN ) ) ) )
        {
            _overlayPositions[i] = 0;

            if ( ( !_playerControllers[0] || !_overlayPositions[0] )
                    && ( !_playerControllers[1] || !_overlayPositions[1] ) )
            {
                _overlayPositions[0] = 0;
                _overlayPositions[1] = 0;
                DllOverlayUi::disable();
                KeyboardManager::get().unhook();
                AsmHacks::enableEscapeToExit = true;
                return;
            }
        }

        // Update overlay text with all the options
        for ( const string& option : options )
            playerText += "\n" + option;

        // Filter keyboard overlay controls when mapping directions
        if ( _playerControllers[i]->isKeyboard() && _playerControllers[i]->isMapping()
                && _overlayPositions[i] >= 1 && _overlayPositions[i] <= 4 )
        {
            DllOverlayUi::updateSelector ( i, headerHeight + _overlayPositions[i], options[_overlayPositions[i]] );
            continue;
        }

        bool deleteMapping = false, mapDirections = false, changedPosition = false;

        if ( ( i == 0
                && ( ( _playerControllers[i]->isJoystick() && isDirectionPressed ( _playerControllers[i], 4 ) )
                     || ( _playerControllers[i]->isKeyboard() && KeyboardState::isPressed ( VK_LEFT ) ) ) )
                || ( i == 1
                     && ( ( _playerControllers[i]->isJoystick() && isDirectionPressed ( _playerControllers[i], 6 ) )
                          || ( _playerControllers[i]->isKeyboard() && KeyboardState::isPressed ( VK_RIGHT ) ) ) ) )
        {
            // Delete selected mapping
            deleteMapping = true;
        }
        else if ( _playerControllers[i]->isKeyboard()
                  && ( KeyboardState::isReleased ( VK_RETURN )      // Use Return key released to prevent the
                       || KeyboardState::isPressed ( VK_DELETE ) )  // same key event from being mapped immediately.
                  && _overlayPositions[i] >= 1 && _overlayPositions[i] <= 4 )
        {
            // Press enter / delete to modify direction keys
            if ( KeyboardState::isReleased ( VK_RETURN ) )
                mapDirections = true;
            else
                deleteMapping = true;
        }
        else if ( ( _playerControllers[i]->isJoystick() && isDirectionPressed ( _playerControllers[i], 2 ) )
                  || ( _playerControllers[i]->isKeyboard() && KeyboardState::isPressedOrHeld ( VK_DOWN ) ) )
        {
            // Move selector down
            _overlayPositions[i] = ( _overlayPositions[i] + 1 ) % options.size();
            changedPosition = true;
        }
        else if ( ( _playerControllers[i]->isJoystick() && isDirectionPressed ( _playerControllers[i], 8 ) )
                  || ( _playerControllers[i]->isKeyboard() && KeyboardState::isPressedOrHeld ( VK_UP ) ) )
        {
            // Move selector up
            _overlayPositions[i] = ( _overlayPositions[i] + options.size() - 1 ) % options.size();
            changedPosition = true;
        }

        if ( deleteMapping || mapDirections || changedPosition || _finishedMapping[i] )
        {
            if ( _overlayPositions[i] >= 1 && _overlayPositions[i] < gameInputBits.size() + 1 )
            {
                // Convert selector position to game input bit position
                const size_t pos = _overlayPositions[i] - 1 + ( _playerControllers[i]->isKeyboard() ? 0 : 4 );

                if ( deleteMapping && pos < gameInputBits.size() )
                {
                    // Delete mapping
                    _playerControllers[i]->clearMapping ( gameInputBits[pos].second );
                    saveMappings ( _playerControllers[i] );
                }
                else if ( pos >= 4 && pos < gameInputBits.size() )
                {
                    // Map a button only
                    _playerControllers[i]->startMapping ( this, gameInputBits[pos].second,
                                                          MAP_CONTINUOUSLY | MAP_PRESERVE_DIRS );
                }
                else if ( ( mapDirections || _finishedMapping[i] ) && pos < 4 )
                {
                    ASSERT ( _playerControllers[i]->isKeyboard() == true );

                    // Map a keyboard direction
                    _playerControllers[i]->startMapping ( this, gameInputBits[pos].second );
                }
                else
                {
                    // In all other situations cancel the current mapping
                    _playerControllers[i]->cancelMapping();
                }
            }
            else
            {
                // In all other situations cancel the current mapping
                _playerControllers[i]->cancelMapping();
            }

            _finishedMapping[i] = false;
        }

        if ( _overlayPositions[i] == 0 )
        {
            playerText = string ( "Press Up or Down to set keys" )
                         + string ( controllersHeight, '\n' )
                         + _playerControllers[i]->getName();
            DllOverlayUi::updateSelector ( i, controllersHeight, _playerControllers[i]->getName() );
        }
        else
        {
            DllOverlayUi::updateSelector ( i, headerHeight + _overlayPositions[i], options[_overlayPositions[i]] );
        }
    }

    DllOverlayUi::updateText ( text );

    // Enable Escape to exit if neither controller is being mapped
    AsmHacks::enableEscapeToExit =
        ( !_playerControllers[0] || _overlayPositions[0] == 0 )
        && ( !_playerControllers[1] || _overlayPositions[1] == 0 );

    ControllerManager::get().savePrevStates();
}

void DllControllerManager::keyboardEvent ( uint32_t vkCode, uint32_t scanCode, bool isExtended, bool isDown )
{
    Lock lock ( ControllerManager::get().mutex );

    for ( uint8_t i = 0; i < 2; ++i )
    {
        if ( _playerControllers[i] && _playerControllers[i]->isKeyboard()
                && _overlayPositions[i] >= 1 && _overlayPositions[i] < gameInputBits.size() + 1 )
        {
            // Convert selector position to game input bit position
            const size_t pos = _overlayPositions[i] - 1;

            // Ignore specific control keys when mapping keyboard buttons
            if ( pos >= 4 && pos < gameInputBits.size()
                    && ( vkCode == VK_TOGGLE_OVERLAY
                         || vkCode == VK_ESCAPE
                         || vkCode == VK_UP
                         || vkCode == VK_DOWN
                         || vkCode == VK_LEFT
                         || vkCode == VK_RIGHT ) )
            {
                return;
            }

            _playerControllers[i]->keyboardEvent ( vkCode, scanCode, isExtended, isDown );
            return;
        }
    }
}

void DllControllerManager::joystickAttached ( Controller *controller )
{
    // This is a callback from within ControllerManager, so we don't need to lock the main mutex

    _allControllers.push_back ( controller );

    _controllerAttached = true;
}

void DllControllerManager::joystickToBeDetached ( Controller *controller )
{
    // This is a callback from within ControllerManager, so we don't need to lock the main mutex

    if ( _playerControllers[0] == controller )
    {
        _playerControllers[0] = 0;
        _overlayPositions[0] = 0;
    }

    if ( _playerControllers[1] == controller )
    {
        _playerControllers[1] = 0;
        _overlayPositions[1] = 0;
    }

    const auto it = find ( _allControllers.begin(), _allControllers.end(), controller );

    ASSERT ( it != _allControllers.end() );

    _allControllers.erase ( it );
}

void DllControllerManager::controllerKeyMapped ( Controller *controller, uint32_t key )
{
    // This is a callback from within ControllerManager, so we don't need to lock the main mutex

    LOG ( "%s: controller=%08x; key=%08x", controller->getName(), controller, key );

    if ( key )
    {
        saveMappings ( controller );

        for ( uint8_t i = 0; i < 2; ++i )
        {
            if ( controller == _playerControllers[i]
                    && _overlayPositions[i] >= 1 && _overlayPositions[i] < gameInputBits.size() + 1  )
            {
                // Convert selector position to game input bit position
                const size_t pos = _overlayPositions[i] - 1 + ( _playerControllers[i]->isKeyboard() ? 0 : 4 );

                // Continue mapping
                if ( pos + 1 < gameInputBits.size() )
                    ++_overlayPositions[i];

                _finishedMapping[i] = true;
                return;
            }
        }
    }
}
