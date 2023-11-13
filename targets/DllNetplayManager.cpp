#include "DllNetplayManager.hpp"
#include "DllAsmHacks.hpp"
#include "ProcessManager.hpp"
#include "Exceptions.hpp"
#include "CharacterSelect.hpp"
#include "ReplayCreator.hpp"
// #include "DllTrialManager.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <ctime>
#include <fstream>

using namespace std;


// Max allowed retry menu index (once again, chara select, save replay).
// Prevent returning to main menu
#define MAX_RETRY_MENU_INDEX ( 2 )

// Extra number to add to preserveStartIndex, this is a safety buffer for chained spectators.
#define PRESERVE_START_INDEX_BUFFER ( 5 )


#define RETURN_MASH_INPUT(DIRECTION, BUTTONS)                       \
    do {                                                            \
        if ( getFrame() % 2 )                                       \
            return 0;                                               \
        return COMBINE_INPUT ( ( DIRECTION ), ( BUTTONS ) );        \
    } while ( 0 )


uint16_t NetplayManager::getPreInitialInput ( uint8_t player )
{
    if ( ( *CC_GAME_MODE_ADDR ) == CC_GAME_MODE_MAIN )
        return 0;

    AsmHacks::menuConfirmState = 2;
    RETURN_MASH_INPUT ( 0, CC_BUTTON_CONFIRM );
}

uint16_t NetplayManager::getInitialInput ( uint8_t player )
{
    if ( ( *CC_GAME_MODE_ADDR ) != CC_GAME_MODE_MAIN )
        return getPreInitialInput ( player );

    // The host player should select the main menu, so that the host controls training mode
    if ( player != config.hostPlayer )
        return 0;

    // Wait until we know what game mode to go to
    if ( config.mode.isUnknown() )
        return 0;

    AsmHacks::menuConfirmState = 2;
    RETURN_MASH_INPUT ( 0, CC_BUTTON_CONFIRM );
}

uint16_t NetplayManager::getAutoCharaSelectInput ( uint8_t player )
{
    *CC_P1_CHARA_SELECTOR_ADDR = ( uint32_t ) charaToSelector ( initial.chara[0] );
    *CC_P2_CHARA_SELECTOR_ADDR = ( uint32_t ) charaToSelector ( initial.chara[1] );

    *CC_P1_CHARACTER_ADDR = ( uint32_t ) initial.chara[0];
    *CC_P2_CHARACTER_ADDR = ( uint32_t ) initial.chara[1];

    *CC_P1_MOON_SELECTOR_ADDR = ( uint32_t ) initial.moon[0];
    *CC_P2_MOON_SELECTOR_ADDR = ( uint32_t ) initial.moon[1];

    *CC_P1_COLOR_SELECTOR_ADDR = ( uint32_t ) initial.color[0];
    *CC_P2_COLOR_SELECTOR_ADDR = ( uint32_t ) initial.color[1];

    *CC_STAGE_SELECTOR_ADDR = initial.stage;

    RETURN_MASH_INPUT ( 0, CC_BUTTON_CONFIRM );
}

uint16_t NetplayManager::getCharaSelectInput ( uint8_t player )
{
    uint16_t input = getRawInput ( player );

    // Prevent hitting Confirm until 150f after beginning of CharaSelect, this is to workaround the moon selector desync
    if ( config.mode.isOnline() && getFrame() < 150 )
    {
        input &= ~ COMBINE_INPUT ( 0, CC_BUTTON_A | CC_BUTTON_CONFIRM );
    }

    // Prevent exiting character select
    if ( ( * ( player == 1 ? CC_P1_SELECTOR_MODE_ADDR : CC_P2_SELECTOR_MODE_ADDR ) ) == CC_SELECT_CHARA )
    {
        input &= ~ COMBINE_INPUT ( 0, CC_BUTTON_B | CC_BUTTON_CANCEL );
    }

    // Don't allow hitting Confirm or Cancel until 2f after we have stopped changing the selector mode
    if ( hasButtonInHistory ( player, CC_BUTTON_A | CC_BUTTON_CONFIRM | CC_BUTTON_B | CC_BUTTON_CANCEL, 1, 3 ) )
    {
        input &= ~ COMBINE_INPUT ( 0, CC_BUTTON_A | CC_BUTTON_CONFIRM | CC_BUTTON_B | CC_BUTTON_CANCEL );
    }

    return input;
}

uint16_t NetplayManager::getSkippableInput ( uint8_t player )
{
    if ( config.mode.isSpectate() )
        RETURN_MASH_INPUT ( 0, CC_BUTTON_CONFIRM );

    if ( config.mode.isReplay() ) {
        uint16_t input = getRawInput ( player );
        AsmHacks::menuConfirmState = 2;
        if ( *CC_PAUSE_FLAG_ADDR )
        {
            AsmHacks::menuConfirmState = 2;

            // Don't allow hitting Confirm until 3f after we have stopped moving the cursor. This is a work around
            // for the issue when select is pressed after the cursor moves, but before currentMenuIndex is updated.
            if ( hasUpDownInHistory ( player, 0, 3 ) )
                input &= ~ COMBINE_INPUT ( 0, CC_BUTTON_A | CC_BUTTON_CONFIRM );
        }
        return input;
    }
    // Only allow the confirm and cancel here
    return ( getRawInput ( player ) & COMBINE_INPUT ( 0, CC_BUTTON_CONFIRM | CC_BUTTON_CANCEL ) );
}

// uint16_t NetplayManager::getDemoInput ( uint8_t player )
// {
//     if ( player == 2 ) {
//         return 0;
//     }
//     uint16_t playerInput = getRawInput( player );
//     if ( playerInput != COMBINE_INPUT( 0, 0 ) &&
//          playerInput != COMBINE_INPUT( 0, CC_PLAYER_FACING ) ) {
//         TrialManager::playDemo = false;
//         TrialManager::demoPosition = 0;
//         demoCountdown = 60;
//         exitCountdown = 0;
//         return playerInput;
//     }
//     LOG("demoinput");
//     Trial currentTrial = TrialManager::charaTrials[TrialManager::currentTrialIndex];
//     if ( exitCountdown > 0 ) {
//         exitCountdown--;
//         if ( exitCountdown == 20 ) {
//             * ( player == 1 ? CC_P1_X_POSITION_ADDR : CC_P2_X_POSITION_ADDR ) = currentTrial.startingPositions[0];
//             * ( player == 1 ? CC_P2_X_POSITION_ADDR : CC_P1_X_POSITION_ADDR ) = currentTrial.startingPositions[1];
//         }
//         if ( exitCountdown == 0 )
//             TrialManager::playDemo = false;
//         return 0;
//     }
//     if ( !currentTrial.demoInputs.size() ) {
//         LOG("nodemo");
//         TrialManager::playDemo = false;
//         return 0;
//     }
//     if ( TrialManager::demoPosition == 0 && demoCountdown > 0 ) {
//         LOG("democountdown@%d", demoCountdown);
//         uint16_t input = 0;
//         if ( demoCountdown == 55 ) {
//             input = COMBINE_INPUT ( 0, CC_BUTTON_FN2 );
//         } else if ( demoCountdown == 50) {
//             LOG( "%d, %d", currentTrial.startingPositions[0], currentTrial.startingPositions[1]);
//             * ( player == 1 ? CC_P1_X_POSITION_ADDR : CC_P2_X_POSITION_ADDR ) = currentTrial.startingPositions[0];
//             * ( player == 1 ? CC_P2_X_POSITION_ADDR : CC_P1_X_POSITION_ADDR ) = currentTrial.startingPositions[1];
//         }
//         demoCountdown--;
//         return input;
//     }
//     uint16_t input = currentTrial.demoInputs[TrialManager::demoPosition++];
//     LOG("input %d@%d", input, TrialManager::demoPosition );
//     if ( TrialManager::demoPosition >= (int) currentTrial.demoInputs.size() ) {
//         TrialManager::demoPosition = 0;
//         demoCountdown = 60;
//         exitCountdown = 30;
//         input = COMBINE_INPUT ( 0, CC_BUTTON_FN2 );
//     }
//     return input;
// }

uint16_t NetplayManager::getInGameInput ( uint8_t player )
{
    uint16_t input = getRawInput ( player );

    // Disable pausing in netplay versus mode. Also only allow start button in versus after holding it for a duration.
    if ( ( ( ( config.mode.isNetplay() && config.mode.isVersus() ) || config.mode.isSpectate() )
            || ( ! *CC_PAUSE_FLAG_ADDR
                 && config.mode.isVersus()
                 && heldStartDuration
                 && ! heldButtonInHistory ( player, CC_BUTTON_START, 0, heldStartDuration ) ) )
	    && ( !config.mode.isReplay() ) )
    {
        input &= ~ COMBINE_INPUT ( 0, CC_BUTTON_START );
    }

    // If the pause menu is up
    if ( *CC_PAUSE_FLAG_ADDR )
    {
        // TrialManager::hideText = true;
        AsmHacks::menuConfirmState = 2;

        // Don't allow hitting Confirm until 3f after we have stopped moving the cursor. This is a work around
        // for the issue when select is pressed after the cursor moves, but before currentMenuIndex is updated.
        if ( hasUpDownInHistory ( player, 0, 3 ) )
            input &= ~ COMBINE_INPUT ( 0, CC_BUTTON_A | CC_BUTTON_CONFIRM );

        // Disable returning to main menu; 16 and 6 are the menu positions for training and versus mode respectively
        if ( AsmHacks::currentMenuIndex == ( config.mode.isTraining() ? 16 : ( config.mode.isReplay() ? 11 : 6 ) ) )
            input &= ~ COMBINE_INPUT ( 0, CC_BUTTON_A | CC_BUTTON_CONFIRM );

    }
    else if ( config.mode.isTraining() && config.mode.isOffline() && player == config.hostPlayer
              && *CC_DUMMY_STATUS_ADDR != CC_DUMMY_STATUS_DUMMY
              && *CC_DUMMY_STATUS_ADDR != CC_DUMMY_STATUS_RECORD )
    {
        // TrialManager::hideText = false;
        // Training mode enhancements when not paused
        if ( _trainingResetState == -1 && ( input & COMBINE_INPUT ( 0, CC_BUTTON_FN2 ) ) )  // Initial reset input
        {
            _trainingResetState = 0;

            if ( ( 0xF & input ) == 4 )
                _trainingResetType = -1;
            else if ( ( 0xF & input ) == 6 )
                _trainingResetType = 1;
            else if ( ( 0xF & input ) == 2 )
                _trainingResetType = 2;
            else
                _trainingResetType = 0;

            input |= COMBINE_INPUT ( 0, CC_BUTTON_FN2 );
            // TrialManager::comboTrialPosition = 0;
            *CC_P1_COMBO_GUARD_ADDR = 50;
        }
        else if ( ( _trainingResetState == -2 || _trainingResetState >= 0 )
                  && ! ( input & COMBINE_INPUT ( 0, CC_BUTTON_FN2 ) ) )                     // Completed reset
        {
            _trainingResetState = -1;
        }
        else if ( _trainingResetState > 10 )                                                // Check for held reset
        {
            // Swap sides if reset button is held for 10f
            if ( input & COMBINE_INPUT ( 0, CC_BUTTON_FN2 ) )
            {
                if ( _trainingResetType == 0 )
                    swap ( *CC_P1_X_POSITION_ADDR , *CC_P2_X_POSITION_ADDR );
                else if ( _trainingResetType == -1 )
                    * ( player == 1 ? CC_P1_X_POSITION_ADDR : CC_P2_X_POSITION_ADDR ) = -65536;
                else if ( _trainingResetType == 1 )
                    * ( player == 1 ? CC_P1_X_POSITION_ADDR : CC_P2_X_POSITION_ADDR ) = 65536;

                if ( _trainingResetType != 2 )
                    swap ( *CC_P1_FACING_FLAG_ADDR , *CC_P2_FACING_FLAG_ADDR );
            }

            _trainingResetState = -2;

            input |= COMBINE_INPUT ( 0, CC_BUTTON_FN2 );
        }
        else if ( _trainingResetState >= 0 )                                                // Reset in progress
        {
            if ( _trainingResetType == -1 )
            {
                * ( player == 1 ? CC_P1_X_POSITION_ADDR : CC_P2_X_POSITION_ADDR ) = -45056;
                * ( player == 1 ? CC_P2_X_POSITION_ADDR : CC_P1_X_POSITION_ADDR ) = -61440;
                *CC_CAMERA_X_ADDR = -26624;
            }
            else if ( _trainingResetType == 1 )
            {
                * ( player == 1 ? CC_P1_X_POSITION_ADDR : CC_P2_X_POSITION_ADDR ) = 45056;
                * ( player == 1 ? CC_P2_X_POSITION_ADDR : CC_P1_X_POSITION_ADDR ) = 61440;
                *CC_CAMERA_X_ADDR = 26624;
            }
            else if ( _trainingResetType == 2 )
            {
                * ( player == 1 ? CC_P1_X_POSITION_ADDR : CC_P2_X_POSITION_ADDR ) = 16384;
                * ( player == 1 ? CC_P2_X_POSITION_ADDR : CC_P1_X_POSITION_ADDR ) = -16384;
            }

            ++_trainingResetState;

            input |= COMBINE_INPUT ( 0, CC_BUTTON_FN2 );
        }
    }

    return input;
}

uint16_t NetplayManager::getReplayMenuInput ( uint8_t player )
{
    uint16_t input = getRawInput(player);

    // Prevent exiting character select
    input &= ~ COMBINE_INPUT ( 0, CC_BUTTON_B | CC_BUTTON_CANCEL );

    return input;
}

uint16_t NetplayManager::getRetryMenuInput ( uint8_t player )
{
    // Ignore remote input on netplay
    if ( player != _localPlayer && config.mode.isNetplay() )
        return 0;

    // Auto navigate when final retry menu index has been decided
    if ( _targetMenuState != -1 && _targetMenuIndex != -1 )
        return getMenuNavInput();

    uint16_t input;

    if ( config.mode.isSpectateNetplay() )
    {
        input = ( ( getFrame() % 2 ) ? 0 : COMBINE_INPUT ( 0, CC_BUTTON_CONFIRM ) );
    }
    else
    {
        input = getRawInput ( player );

        // Don't allow hitting Confirm until 3f after we have stopped moving the cursor. This is a work around
        // for the issue when select is pressed after the cursor moves, but before currentMenuIndex is updated.
        if ( hasUpDownInHistory ( config.mode.isNetplay() ? player : 0, 0, 3 ) )
            input &= ~ COMBINE_INPUT ( 0, CC_BUTTON_A | CC_BUTTON_CONFIRM );

        // Limit retry menu selectable options
        if ( AsmHacks::currentMenuIndex > MAX_RETRY_MENU_INDEX )
            input &= ~ COMBINE_INPUT ( 0, CC_BUTTON_A | CC_BUTTON_CONFIRM );
    }

    // Check if auto replay save is enabled
    if ( *CC_AUTO_REPLAY_SAVE_ADDR && AsmHacks::autoReplaySaveStatePtr )
    {
        // Prevent mashing through the auto replay save and causing a hang
        if ( *AsmHacks::autoReplaySaveStatePtr == 100 )
            return 0;
    }

    // Allow saving replays; when manual replay save is selected or any replay save menu is open
    if ( AsmHacks::currentMenuIndex == 2 || *CC_MENU_STATE_COUNTER_ADDR > _retryMenuStateCounter )
    {
        AsmHacks::menuConfirmState = 2;
        return input;
    }

    if ( config.mode.isSpectateNetplay() )
    {
        // Disable menu confirms
        AsmHacks::menuConfirmState = 0;

        // Get the retry menu index for the current transition index
        MsgPtr msgMenuIndex = getRetryMenuIndex ( getIndex() );

        // Check if we're done auto-saving (or not auto-saving)
        const bool doneAutoSave = ! ( *CC_AUTO_REPLAY_SAVE_ADDR )
                                  || ( AsmHacks::autoReplaySaveStatePtr && *AsmHacks::autoReplaySaveStatePtr > 100 );

        // Navigate the menu when the menu index is ready AND we're done auto-saving
        if ( msgMenuIndex && doneAutoSave )
        {
            _targetMenuState = 0;
            _targetMenuIndex = msgMenuIndex->getAs<MenuIndex>().menuIndex;
            return 0;
        }
    }
    else if ( config.mode.isNetplay() )
    {
        // Special netplay retry menu behaviour, only select final option after both sides have selected
        if ( _remoteRetryMenuIndex != -1 && _localRetryMenuIndex != -1 )
        {
            _targetMenuState = 0;
            _targetMenuIndex = max ( _localRetryMenuIndex, _remoteRetryMenuIndex );
            _targetMenuIndex = min ( _targetMenuIndex, ( int8_t ) 1 ); // Just in case...
            setRetryMenuIndex ( getIndex(), _targetMenuIndex );
            input = 0;
        }
        else if ( _localRetryMenuIndex != -1 )
        {
            input = 0;
        }
        else if ( AsmHacks::menuConfirmState == 1 )
        {
            _localRetryMenuIndex = AsmHacks::currentMenuIndex;
            input = 0;

            LOG ( "localRetryMenuIndex=%d", _localRetryMenuIndex );
        }

        // Disable menu confirms
        AsmHacks::menuConfirmState = 0;
    }
    else
    {
        // Allow regular retry menu operation when not netplaying
        AsmHacks::menuConfirmState = 2;
    }

    return input;
}

MsgPtr NetplayManager::getLocalRetryMenuIndex() const
{
    if ( _state == NetplayState::RetryMenu && _localRetryMenuIndex != -1 )
        return MsgPtr ( new MenuIndex ( getIndex(), _localRetryMenuIndex ) );
    else
        return 0;
}

void NetplayManager::setRemoteRetryMenuIndex ( int8_t menuIndex )
{
    _remoteRetryMenuIndex = menuIndex;

    LOG ( "remoteRetryMenuIndex=%d", _remoteRetryMenuIndex );
}

MsgPtr NetplayManager::getRetryMenuIndex ( uint32_t index ) const
{
    if ( config.mode.isOffline() )
        return 0;

    LOG ( "[%s] index=%u", _indexedFrame, index );

    ASSERT ( index >= _startIndex );

    if ( index >= _startIndex + _retryMenuIndicies.size() )
        return 0;

    if ( _retryMenuIndicies[index - _startIndex] < 0 )
        return 0;

    return MsgPtr ( new MenuIndex ( index, _retryMenuIndicies[index - _startIndex] ) );
}

void NetplayManager::setRetryMenuIndex ( uint32_t index, int8_t menuIndex )
{
    if ( config.mode.isOffline() || index == 0 || index < _startIndex || menuIndex < 0 )
        return;

    LOG ( "[%s] index=%u; menuIndex=%d", _indexedFrame, index, menuIndex );

    ASSERT ( index >= _startIndex );

    if ( index >= _startIndex + _retryMenuIndicies.size() )
        _retryMenuIndicies.resize ( index + 1 - _startIndex, -1 );

    _retryMenuIndicies[index - _startIndex] = menuIndex;
}

uint16_t NetplayManager::getMenuNavInput()
{
    if ( _targetMenuState == -1 || _targetMenuIndex == -1 )
        return 0;

    if ( _targetMenuState == 0 )                                            // Determined targetMenuIndex
    {
        LOG ( "targetMenuIndex=%d", _targetMenuIndex );

        _targetMenuState = 1;
    }
    else if ( _targetMenuState == 1 )                                       // Move up or down towards targetMenuIndex
    {
        _targetMenuState = 2;

        if ( _targetMenuIndex != ( int8_t ) AsmHacks::currentMenuIndex )
            return COMBINE_INPUT ( ( _targetMenuIndex < ( int8_t ) AsmHacks::currentMenuIndex ? 8 : 2 ), 0 );
    }
    else if ( _targetMenuState >= 2 && _targetMenuState <= 4 )              // Wait for currentMenuIndex to update
    {
        ++_targetMenuState;
    }
    else if ( _targetMenuState == 39 )                                      // Mash final menu selection
    {
        AsmHacks::menuConfirmState = 2;
        RETURN_MASH_INPUT ( 0, CC_BUTTON_CONFIRM );
    }
    else if ( _targetMenuIndex != ( int8_t ) AsmHacks::currentMenuIndex )   // Keep navigating
    {
        _targetMenuState = 1;
    }
    else                                                                    // Reached targetMenuIndex
    {
        LOG ( "targetMenuIndex=%d; currentMenuIndex=%u", _targetMenuIndex, AsmHacks::currentMenuIndex );

        _targetMenuState = 39;
    }

    return 0;
}

bool NetplayManager::hasUpDownInHistory ( uint8_t player, uint32_t start, uint32_t end ) const
{
    for ( size_t i = start; i < end; ++i )
    {
        if ( i > getFrame() )
            break;

        if ( player == 0 )
        {
            const uint16_t p1dir = 0xF & getRawInput ( 1, getFrame() - i );

            const uint16_t p2dir = 0xF & getRawInput ( 2, getFrame() - i );

            if ( ( p1dir == 2 ) || ( p1dir == 8 ) || ( p2dir == 2 ) || ( p2dir == 8 ) )
                return true;
        }
        else
        {
            ASSERT ( player == 1 || player == 2 );

            const uint16_t dir = 0xF & getRawInput ( player, getFrame() - i );

            if ( ( dir == 2 ) || ( dir == 8 ) )
                return true;
        }
    }

    return false;
}

bool NetplayManager::hasButtonInHistory ( uint8_t player, uint16_t button, uint32_t start, uint32_t end ) const
{
    ASSERT ( player == 1 || player == 2 );

    for ( size_t i = start; i < end; ++i )
    {
        if ( i > getFrame() )
            break;

        const uint16_t buttons = getRawInput ( player, getFrame() - i ) >> 4;

        if ( buttons & button )
            return true;
    }

    return false;
}

bool NetplayManager::heldButtonInHistory ( uint8_t player, uint16_t button, uint32_t start, uint32_t end ) const
{
    ASSERT ( player == 1 || player == 2 );

    for ( size_t i = start; i < end; ++i )
    {
        if ( i > getFrame() )
            return false;

        const uint16_t buttons = getRawInput ( player, getFrame() - i ) >> 4;

        if ( ! ( buttons & button ) )
            return false;
    }

    return true;
}

void NetplayManager::setRemotePlayer ( uint8_t player )
{
    ASSERT ( player == 1 || player == 2 );

    _localPlayer = 3 - player;
    _remotePlayer = player;
}

void NetplayManager::updateFrame()
{
    _indexedFrame.parts.frame = ( *CC_WORLD_TIMER_ADDR ) - _startWorldTime;
}

uint32_t NetplayManager::getBufferedPreserveStartIndex() const
{
    if ( preserveStartIndex == UINT_MAX )
        return UINT_MAX;

    if ( preserveStartIndex <= PRESERVE_START_INDEX_BUFFER )
        return 0;

    return ( preserveStartIndex - PRESERVE_START_INDEX_BUFFER );
}

void NetplayManager::setState ( NetplayState state )
{
    if ( ! isValidNext ( state ) )
    {
        LOG ( "Invalid transition: %s -> %s", _state, state );
        return;
    }

    LOG ( "indexedFrame=[%s]; previous=%s; current=%s", _indexedFrame, _state, state );

    if ( state.value >= NetplayState::CharaSelect )
    {
        if ( _state == NetplayState::AutoCharaSelect )
        {
            // Start from the initial index and frame
            _startWorldTime = 0;
            *CC_WORLD_TIMER_ADDR = initial.indexedFrame.parts.frame;
            _indexedFrame = initial.indexedFrame;
        }
        else
        {
            // Increment the index whenever the NetplayState changes
            ++_indexedFrame.parts.index;

            // Start counting from frame=0 again
            _startWorldTime = *CC_WORLD_TIMER_ADDR;
            _indexedFrame.parts.frame = 0;
        }

        // Entering CharaSelect
        if ( state == NetplayState::CharaSelect )
            _spectateStartIndex = getIndex();

        // Entering Loading
        if ( state == NetplayState::Loading )
        {
            _spectateStartIndex = getIndex();

            const uint32_t newStartIndex = min ( getBufferedPreserveStartIndex(), getIndex() );

            if ( newStartIndex > _startIndex )
            {
                const size_t offset = newStartIndex - _startIndex;

                _inputs[0].eraseIndexOlderThan ( offset );
                _inputs[1].eraseIndexOlderThan ( offset );

                if ( offset >= _rngStates.size() )
                    _rngStates.clear();
                else
                    _rngStates.erase ( _rngStates.begin(), _rngStates.begin() + offset );

                if ( offset >= _retryMenuIndicies.size() )
                    _retryMenuIndicies.clear();
                else
                    _retryMenuIndicies.erase ( _retryMenuIndicies.begin(), _retryMenuIndicies.begin() + offset );

                _startIndex = newStartIndex;
            }

            _localRetryMenuIndex = -1;
            _remoteRetryMenuIndex = -1;
        }

        // Entering Game
        if ( state == NetplayState::InGame )
        {
            inGameIndexes[ getIndex() - _startIndex ] = true;
            RngState *rngState = new RngState ( 0 );
            rngState->rngState0 = *CC_RNG_STATE0_ADDR;
            rngState->rngState1 = *CC_RNG_STATE1_ADDR;
            rngState->rngState2 = *CC_RNG_STATE2_ADDR;
            copy ( CC_RNG_STATE3_ADDR, CC_RNG_STATE3_ADDR + CC_RNG_STATE3_SIZE, rngState->rngState3.begin() );
            _roundRngStates.push_back(rngState);
        }

        // Entering RetryMenu
        if ( state == NetplayState::RetryMenu )
        {
            // The actual retry menu is opened at position *CC_MENU_STATE_COUNTER_ADDR + 1
            _retryMenuStateCounter = *CC_MENU_STATE_COUNTER_ADDR + 1;
            if ( !config.mode.isSpectate() )
                exportResults();
        }

        // Exiting RetryMenu
        if ( _state == NetplayState::RetryMenu )
        {
            AsmHacks::autoReplaySaveStatePtr = 0;
            resetInGameIndexes();
        }

        // Reset state variables
        AsmHacks::currentMenuIndex = 0;
        AsmHacks::menuConfirmState = 0;
        _targetMenuState = -1;
        _targetMenuIndex = -1;
    }

    _state = state;
}

uint16_t NetplayManager::getInput ( uint8_t player )
{
    ASSERT ( player == 1 || player == 2 );

    switch ( _state.value )
    {
        case NetplayState::PreInitial:
            return getPreInitialInput ( player );

        case NetplayState::Initial:
            return getInitialInput ( player );

        case NetplayState::AutoCharaSelect:
            return getAutoCharaSelectInput ( player );

        case NetplayState::CharaSelect:
            return getCharaSelectInput ( player );

        case NetplayState::Loading:
        case NetplayState::CharaIntro:
        case NetplayState::Skippable:
            // If the remote index is ahead, then we should mash to skip.
            if ( ( _startIndex + _inputs[_remotePlayer - 1].getEndIndex() ) > getIndex() + 1 )
            {
                AsmHacks::menuConfirmState = 2;
                RETURN_MASH_INPUT ( 0, CC_BUTTON_CONFIRM );
            }
            return getSkippableInput ( player );

        case NetplayState::InGame:
            // if ( TrialManager::playDemo ) {
            //     return getDemoInput ( player );
            // }
            return getInGameInput ( player );

        case NetplayState::RetryMenu:
            return getRetryMenuInput ( player );
        case NetplayState::ReplayMenu:
            return getReplayMenuInput ( player );

        default:
            ASSERT_IMPOSSIBLE;
            return 0;
    }
}

uint16_t NetplayManager::getRawInput ( uint8_t player, uint32_t frame ) const
{
    ASSERT ( player == 1 || player == 2 );
    ASSERT ( getIndex() >= _startIndex );

    return _inputs[player - 1].get ( getIndex() - _startIndex, frame );
}

void NetplayManager::setInput ( uint8_t player, uint16_t input )
{
    ASSERT ( player == 1 || player == 2 );
    ASSERT ( getIndex() >= _startIndex );

    if ( isInRollback() ) {
        _inputs[player - 1].set ( getIndex() - _startIndex, getFrame() + config.rollbackDelay, input );
    } else if ( _state == NetplayState::RetryMenu ) {
        _inputs[player - 1].set ( getIndex() - _startIndex, getFrame(), input );
    } else if ( config.mode.isOffline() && splitDelay ) {
        _inputs[player - 1].set ( getIndex() - _startIndex,
                                  getFrame() + (player == 1 ? config.delay : config.rollbackDelay),
                                  input );
    } else {
        _inputs[player - 1].set ( getIndex() - _startIndex, getFrame() + config.delay, input );
    }
}

void NetplayManager::assignInput ( uint8_t player, uint16_t input, uint32_t frame )
{
    assignInput ( player, input, { frame, getIndex() } );
}

void NetplayManager::assignInput ( uint8_t player, uint16_t input, IndexedFrame _indexedFrame )
{
    ASSERT ( player == 1 || player == 2 );
    LOG( "startindex %d", _startIndex );
    ASSERT ( _indexedFrame.parts.index >= _startIndex );

    _inputs[player - 1].assign ( _indexedFrame.parts.index - _startIndex, _indexedFrame.parts.frame, input );
}

MsgPtr NetplayManager::getInputs ( uint8_t player ) const
{
    ASSERT ( player == 1 || player == 2 );
    ASSERT ( getIndex() >= _startIndex );
    ASSERT ( _inputs[player - 1].getEndFrame ( getIndex() - _startIndex ) >= 1 );

    PlayerInputs *playerInputs = new PlayerInputs ( { _inputs[player - 1].getEndFrame() - 1, getIndex() } );

    ASSERT ( playerInputs->getIndex() >= _startIndex );

    _inputs[player - 1].get ( playerInputs->getIndex() - _startIndex, playerInputs->getStartFrame(),
                              &playerInputs->inputs[0], playerInputs->size() );

    return MsgPtr ( playerInputs );
}

void NetplayManager::setInputs ( uint8_t player, const PlayerInputs& playerInputs )
{
    // Only keep remote inputs at most 1 transition index old, but at least as new as the startIndex
    if ( playerInputs.getIndex() + 1 < getIndex() || playerInputs.getIndex() < _startIndex )
        return;

    ASSERT ( player == 1 || player == 2 );
    ASSERT ( getIndex() >= _startIndex );
    ASSERT ( playerInputs.getIndex() >= _startIndex );

    const uint32_t checkStartingFromIndex = ( isInRollback() ? getIndex() - _startIndex : UINT_MAX );

    _inputs[player - 1].set ( playerInputs.getIndex() - _startIndex, playerInputs.getStartFrame(),
                              &playerInputs.inputs[0], playerInputs.size(), checkStartingFromIndex );
}

MsgPtr NetplayManager::getBothInputs ( IndexedFrame& pos ) const
{
    if ( pos.parts.index > getIndex() )
        return 0;

    IndexedFrame orig = pos;

    ASSERT ( orig.parts.index >= _startIndex );

    // This is most recent frame, in the spectator's transition index, that the spectator is allowed to "see"
    uint32_t commonEndFrame = min ( _inputs[0].getEndFrame ( orig.parts.index - _startIndex ),
                                    _inputs[1].getEndFrame ( orig.parts.index - _startIndex ) );

    if ( orig.parts.index == getIndex() )                   // During the same transition index
    {
        if ( isInRollback() )
        {
            // Add a buffer to the end frame during rollback
            if ( commonEndFrame > 2 * NUM_INPUTS )
                commonEndFrame -= 2 * NUM_INPUTS;
            else
                commonEndFrame = 0;
        }

        if ( orig.parts.frame + 1 <= commonEndFrame )
        {
            // Increment by NUM_INPUTS when behind
            pos.parts.frame += NUM_INPUTS;
        }
        else
        {
            // Otherwise return empty, so the spectator has to wait
            return 0;
        }
    }
    else                                                    // During an older transition index
    {
        if ( orig.parts.frame + 1 <= commonEndFrame )
        {
            // Increment by NUM_INPUTS when behind
            pos.parts.frame += NUM_INPUTS;
        }
        else
        {
            // Since we're at the end of this transition index, increment to the next one
            pos.parts.frame = NUM_INPUTS - 1;
            ++pos.parts.index;

            // Return empty if this transition index has no inputs
            if ( commonEndFrame == 0 )
                return 0;

            // Otherwise get the rest of this transition index
            orig.parts.frame = commonEndFrame - 1;
        }
    }

    BothInputs *bothInputs = new BothInputs ( orig );

    ASSERT ( bothInputs->getIndex() >= _startIndex );

    _inputs[0].get ( bothInputs->getIndex() - _startIndex, bothInputs->getStartFrame(),
                     &bothInputs->inputs[0][0], bothInputs->size() );

    _inputs[1].get ( bothInputs->getIndex() - _startIndex, bothInputs->getStartFrame(),
                     &bothInputs->inputs[1][0], bothInputs->size() );

    return MsgPtr ( bothInputs );
}

void NetplayManager::setBothInputs ( const BothInputs& bothInputs )
{
    // Only keep remote inputs at most 1 transition index old, but at least as new as the startIndex
    if ( bothInputs.getIndex() + 1 < getIndex() || bothInputs.getIndex() < _startIndex )
        return;

    ASSERT ( bothInputs.getIndex() >= _startIndex );

    _inputs[0].set ( bothInputs.getIndex() - _startIndex, bothInputs.getStartFrame(),
                     &bothInputs.inputs[0][0], bothInputs.size() );

    _inputs[1].set ( bothInputs.getIndex() - _startIndex, bothInputs.getStartFrame(),
                     &bothInputs.inputs[1][0], bothInputs.size() );
}

bool NetplayManager::isRemoteInputReady() const
{
    if ( _state.value < NetplayState::CharaSelect || _state.value == NetplayState::Skippable
            || _state.value == NetplayState::Loading || _state.value == NetplayState::RetryMenu
         || _state.value == NetplayState::CharaIntro )
    {
        return true;
    }

    if ( _inputs[_remotePlayer - 1].empty() )
    {
        LOG ( "[%s] No remote inputs (index)", _indexedFrame );
        return false;
    }

    ASSERT ( _inputs[_remotePlayer - 1].getEndIndex() >= 1 );

    if ( _startIndex + _inputs[_remotePlayer - 1].getEndIndex() - 1 < getIndex() )
    {
        LOG ( "[%s] remoteIndex=%u < localIndex=%u",
              _indexedFrame, _startIndex + _inputs[_remotePlayer - 1].getEndIndex() - 1, getIndex() );
        return false;
    }

    // If remote index is ahead, we must be in an older state, so we don't need to wait for inputs
    if ( _startIndex + _inputs[_remotePlayer - 1].getEndIndex() - 1 > getIndex() )
        return true;

    if ( _inputs[_remotePlayer - 1].getEndFrame() == 0 )
    {
        LOG ( "[%s] No remote inputs (frame)", _indexedFrame );
        return false;
    }

    ASSERT ( _inputs[_remotePlayer - 1].getEndFrame() >= 1 );

    const uint8_t maxFramesAhead = ( isInRollback() ? config.rollback : 0 );

    if ( ( _inputs[_remotePlayer - 1].getEndFrame() - 1 + maxFramesAhead ) < getFrame() )
    {
        LOG ( "[%s] remoteFrame = %u < localFrame=%u; delay=%u; rollback=%u; rollbackDelay=%u",
              _indexedFrame, _inputs[_remotePlayer - 1].getEndFrame() - 1, getFrame(),
              config.delay, config.rollback, config.rollbackDelay );

        return false;
    }

    return true;
}

MsgPtr NetplayManager::getRngState ( uint32_t index ) const
{
    if ( config.mode.isOffline() )
        return 0;

    LOG ( "[%s]", _indexedFrame );

    ASSERT ( index >= _startIndex );

    if ( index >= _startIndex + _rngStates.size() )
        return 0;

    return _rngStates[index - _startIndex];
}

void NetplayManager::setRngState ( const RngState& rngState )
{
    if ( config.mode.isOffline() || rngState.index == 0 || rngState.index < _startIndex )
        return;

    LOG ( "[%s] rngState.index=%u", _indexedFrame, rngState.index );

    ASSERT ( rngState.index >= _startIndex );

    if ( rngState.index >= _startIndex + _rngStates.size() )
        _rngStates.resize ( rngState.index + 1 - _startIndex );

    _rngStates[rngState.index - _startIndex].reset ( new RngState ( rngState ) );
}

bool NetplayManager::isRngStateReady ( bool shouldSyncRngState ) const
{
    if ( !shouldSyncRngState
            || config.mode.isHost() || config.mode.isBroadcast() || config.mode.isOffline()
            || _state.value < NetplayState::CharaSelect )
    {
        return true;
    }

    if ( _rngStates.empty() )
    {
        LOG ( "[%s] No remote RngStates", _indexedFrame );
        return false;
    }

    if ( ( _startIndex + _rngStates.size() - 1 ) < getIndex() )
    {
        LOG ( "[%s] remoteIndex=%u < localIndex=%u", _indexedFrame, _startIndex + _rngStates.size() - 1, getIndex() );
        return false;
    }

    return true;
}

uint32_t NetplayManager::getRemoteIndex() const
{
    uint32_t remoteIndex = _inputs[_remotePlayer - 1].getEndIndex() + _startIndex;

    if ( remoteIndex > 0 )
        --remoteIndex;

    return remoteIndex;
}

uint32_t NetplayManager::getRemoteFrame() const
{
    uint32_t remoteFrame = _inputs[_remotePlayer - 1].getEndFrame();

    if ( remoteFrame > 0 )
        --remoteFrame;

    return remoteFrame;
}

IndexedFrame NetplayManager::getRemoteIndexedFrame() const
{
    IndexedFrame _indexedFrame =
    {
        _inputs[_remotePlayer - 1].getEndFrame(),
        _inputs[_remotePlayer - 1].getEndIndex() + _startIndex
    };

    if ( _indexedFrame.parts.frame > 0 )
        --_indexedFrame.parts.frame;

    if ( _indexedFrame.parts.index > 0 )
        --_indexedFrame.parts.index;

    return _indexedFrame;
}

IndexedFrame NetplayManager::getLastChangedFrame() const
{
    IndexedFrame _indexedFrame = _inputs[_remotePlayer - 1].getLastChangedFrame();

    if ( _indexedFrame.value == MaxIndexedFrame.value )
        return MaxIndexedFrame;

    return { _indexedFrame.parts.frame, _startIndex + _indexedFrame.parts.index };
}

void NetplayManager::clearLastChangedFrame()
{
    _inputs[_remotePlayer - 1].clearLastChangedFrame();
}

void NetplayManager::setRemoteIndex ( uint32_t remoteIndex )
{
    if ( remoteIndex < _startIndex )
        return;

    LOG ( "remoteIndex=%u", remoteIndex );

    _inputs[_remotePlayer - 1].resize ( remoteIndex - _startIndex, 0, 0 );
}

bool NetplayManager::isValidNext ( NetplayState next )
{
    static unordered_map<uint8_t, unordered_set<uint8_t>> validTransitions =
    {
        { NetplayState::Unknown, { NetplayState::PreInitial } },
        { NetplayState::PreInitial, { NetplayState::Initial } },
        { NetplayState::Initial, { NetplayState::AutoCharaSelect, NetplayState::CharaSelect, NetplayState::ReplayMenu } },
        { NetplayState::AutoCharaSelect, { NetplayState::Loading } },
        { NetplayState::CharaSelect, { NetplayState::Loading } },
        { NetplayState::Loading, { NetplayState::Skippable, NetplayState::CharaIntro, NetplayState::InGame } },
        { NetplayState::CharaIntro, { NetplayState::InGame } },
        { NetplayState::Skippable, { NetplayState::InGame, NetplayState::RetryMenu } },
        { NetplayState::InGame, { NetplayState::Skippable, NetplayState::CharaSelect, NetplayState::ReplayMenu, NetplayState::RetryMenu } },
        { NetplayState::RetryMenu, { NetplayState::Loading, NetplayState::CharaSelect, NetplayState::ReplayMenu } },
        { NetplayState::ReplayMenu, { NetplayState::Loading } },
    };

    const auto it = validTransitions.find ( _state.value );

    if ( it == validTransitions.end() )
        return false;

    return ( it->second.find ( next.value ) != it->second.end() );
}

void NetplayManager::exportInputs() {
    char buf[1000];
    char namebuf[1000];
    char namebuf2[1000];
    char timebuf[200];

    std::time_t now = time( NULL );
    strftime( timebuf, 20, "%y%m%d-%H%M%S", localtime( &now ) );

    sprintf( namebuf, "ReplayVS/%sx%s_%s.repraw",
             getShortCharaName( *CC_P1_CHARACTER_ADDR ),
             getShortCharaName( *CC_P2_CHARACTER_ADDR) ,
             timebuf );
    ofstream repFile2 ( namebuf, ios::out );
    vector<int> c = getInGameIndexes();
    repFile2 << c.size() << endl;
    for ( int q : c ) {
        sprintf( buf, "%d\n", _inputs[0].getEndFrame( q ) );
        repFile2 << buf;
        for ( uint32_t i = 0; i < _inputs[0].getEndFrame( q ); ++i ) {
            sprintf( buf, "%04x %04x\n",
                     _inputs[0].get ( q, i ),
                     _inputs[1].get ( q, i ) );
            repFile2 << buf;
        }
    }
    repFile2.close();

    sprintf( namebuf2, "%s2.rep", ( char* ) AsmHacks::replayName );
    ReplayCreator::ReplayFile f;
    ReplayCreator r;
    r.load( &f, ( char* )AsmHacks::replayName );
    r.fixReplay( &f, namebuf, NULL );
    r.dump( f, namebuf2 );

    exported = true;
}

void NetplayManager::exportResults()
{
    ofstream resFile;
    resFile.open( "results.csv", ios::out | ios::app );
    char buf[ 1000 + config.names[0].length() + config.names[1].length() ];
    string moon[3] = { "C", "F", "H" };
    std::time_t now = time( NULL );
    string n1 = sanitizePlayerName( config.names[0] );
    string n2 = sanitizePlayerName( config.names[1] );
    if ( _localPlayer == 1 ) {
        sprintf( buf, "%s,%s-%s,%d,%s,%s-%s,%d,%d",
                 n1.c_str(), moon[*CC_P1_MOON_SELECTOR_ADDR].c_str(),
                 getShortCharaName(*CC_P1_CHARACTER_ADDR),
                 *CC_P1_WINS_ADDR,
                 n2.c_str(), moon[*CC_P2_MOON_SELECTOR_ADDR].c_str(),
                 getShortCharaName(*CC_P2_CHARACTER_ADDR),
                 *CC_P2_WINS_ADDR,
                 (int)now
               );
    } else {
        sprintf( buf, "%s,%s-%s,%d,%s,%s-%s,%d,%d",
                 n2.c_str(), moon[*CC_P2_MOON_SELECTOR_ADDR].c_str(),
                 getShortCharaName(*CC_P2_CHARACTER_ADDR),
                 *CC_P2_WINS_ADDR,
                 n1.c_str(), moon[*CC_P1_MOON_SELECTOR_ADDR].c_str(),
                 getShortCharaName(*CC_P1_CHARACTER_ADDR),
                 *CC_P1_WINS_ADDR,
                 (int)now
               );
    }
    resFile << buf << endl;
    resFile.close();
}

void NetplayManager::resetInGameIndexes() {
    for (int i = 0; i < 30; ++i ){
        inGameIndexes[i] = false;
    }
}

vector<int> NetplayManager::getInGameIndexes() {
    vector<int> v;
    for (int i = 0; i < 30; ++i ){
        if ( inGameIndexes[i] ) v.push_back(i);
    }
    return v;
}

string NetplayManager::sanitizePlayerName( string name )
{
    findAndReplaceAll(name, ",", "&comma;");

    return name;
}

void NetplayManager::findAndReplaceAll( string& data, string toSearch, string replaceStr )
{
    size_t pos = data.find(toSearch);

    while( pos != string::npos )
    {
        data.replace(pos, toSearch.size(), replaceStr);
        pos = data.find(toSearch, pos + replaceStr.size());
    }
}

