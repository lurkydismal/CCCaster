#pragma once

#include <climits>
#include <vector>

#include "InputsContainer.hpp"
#include "Messages.hpp"
#include "NetplayStates.hpp"

// Class that manages netplay state and inputs
class NetplayManager {
public:
    // Netplay config
    NetplayConfig config;

    // Initial game state (spectate only)
    InitialGameState initial;

    // Preserve input/RngState/MenuIndex starting from this index
    uint32_t preserveStartIndex = UINT_MAX;

    // The number of frames it takes to register a held start button input
    uint32_t heldStartDuration = 0;

    // Automatically save replays
    uint32_t autoReplaySave = false;

    // Indicate which player is the remote player
    void setRemotePlayer( uint8_t player );

    // Update the current netplay frame
    void updateFrame();

    // Get index / frame information
    uint32_t getFrame() const { return _indexedFrame.parts.frame; }
    uint32_t getIndex() const { return _indexedFrame.parts.index; }
    IndexedFrame getIndexedFrame() const { return _indexedFrame; }
    uint32_t getRemoteIndex() const;
    uint32_t getRemoteFrame() const;
    IndexedFrame getRemoteIndexedFrame() const;

    // Get the delta between local and remote frames, returns 0 if the index is
    // different
    int getRemoteFrameDelta() const {
        if ( getIndex() == getRemoteIndex() )
            return ( int )getFrame() - int( getRemoteFrame() + config.delay -
                                            config.rollbackDelay );

        return 0;
    }

    // Get the index for spectators to start inputs on.
    // During CharaSelect state, this is the beginning of the current
    // CharaSelect state. During any other state, this is the beginning of the
    // current game's Loading state.
    uint32_t getSpectateStartIndex() const { return _spectateStartIndex; }

    // Get / clear the last changed frame (for rollback)
    IndexedFrame getLastChangedFrame() const;
    void clearLastChangedFrame();

    // Dump netplay inputs
    bool inGameIndexes[ 30 ] = {};
    std::vector< int > getInGameIndexes();
    void resetInGameIndexes();
    void exportInputs();

    // Log Results
    void exportResults();

    // Get / set the current NetplayState
    NetplayState getState() const { return _state; }
    void setState( NetplayState state );
    bool isInGame() const { return _state == NetplayState::InGame; }
    bool isInRollback() const {
        return isInGame() && config.rollback && config.mode.isNetplay();
    }

    // Get / set the input for the current frame given the player
    uint16_t getInput( uint8_t player );
    uint16_t getRawInput( uint8_t player ) const {
        return getRawInput( player, getFrame() );
    }
    uint16_t getRawInput( uint8_t player, uint32_t frame ) const;
    void setInput( uint8_t player, uint16_t input );
    void assignInput( uint8_t player, uint16_t input, uint32_t frame );
    void assignInput( uint8_t player,
                      uint16_t input,
                      IndexedFrame indexedFrame );

    // Get / set batch inputs for the given player
    MsgPtr getInputs( uint8_t player ) const;
    void setInputs( uint8_t player, const PlayerInputs& playerInputs );

    // Get inputs both players. May return null if not enough inputs are ready
    // for the given pos. Otherwise this increments the given pos by at most
    // NUM_INPUTS if returning non-null.
    MsgPtr getBothInputs( IndexedFrame& pos ) const;

    // Set inputs for both players
    void setBothInputs( const BothInputs& bothInputs );

    // True if remote input is ready for the current frame, otherwise the caller
    // should wait for more input
    bool isRemoteInputReady() const;

    // Get / set the RngState
    MsgPtr getRngState() const { return getRngState( getIndex() ); }
    MsgPtr getRngState( uint32_t index ) const;
    void setRngState( const RngState& rngState );

    // True if the RngState is ready for the current frame, otherwise the caller
    // should wait for it
    bool isRngStateReady( bool shouldSyncRngState ) const;

    // Get / set the retry menu index
    MsgPtr getLocalRetryMenuIndex() const;
    void setRemoteRetryMenuIndex( int8_t menuIndex );
    MsgPtr getRetryMenuIndex( uint32_t index ) const;
    void setRetryMenuIndex( uint32_t index, int8_t menuIndex );

    // Get / set input delay frames
    uint8_t getDelay() const {
        return ( isInRollback() ? config.rollbackDelay : config.delay );
    }
    uint8_t getRollbackDelay() const { return config.rollbackDelay; }
    void setDelay( uint8_t delay ) {
        if ( isInRollback() )
            config.rollbackDelay = delay;
        else
            config.delay = delay;
    }
    void setRollbackDelay( uint8_t delay ) { config.rollbackDelay = delay; }

    // Get / set input rollback frames
    uint8_t getRollback() const { return config.rollback; }
    void setRollback( uint8_t rollback ) { config.rollback = rollback; }

    // Set remote transition index
    void setRemoteIndex( uint32_t remoteIndex );

    // Check if the next state transition is valid
    bool isValidNext( NetplayState state );

    friend class DllRollbackManager;

private:
    // Netplay state
    NetplayState _state;

    // State of the menu navigation input
    int32_t _targetMenuState = -1;

    // Target menu index to navigate to
    int8_t _targetMenuIndex = -1;

    // Local retry menu selected index
    int8_t _localRetryMenuIndex = -1;

    // Remote retry menu selected index
    int8_t _remoteRetryMenuIndex = -1;

    // State of the training mode reset
    int32_t _trainingResetState = -1;

    // Type of the training mode reset
    int32_t _trainingResetType = 0;

    // The value of *CC_MENU_STATE_COUNTER_ADDR at the beginning of the
    // RetryMenu state. This is used to determine if any other menus are open in
    // front of the retry menu.
    uint32_t _retryMenuStateCounter = 0;

    // The starting value of CC_WORLD_TIMER_ADDR, where frame = (
    // *CC_WORLD_TIMER_ADDR ) - startWorldTime. This is reset to the current
    // world time whenever the NetplayState changes, ie a state transition
    // happens.
    uint32_t _startWorldTime = 0;

    // Current transition index, incremented whenever the NetplayState changes
    // (after CharaSelect). Current netplay frame, frame = (
    // *CC_WORLD_TIMER_ADDR ) - startWorldTime.
    IndexedFrame _indexedFrame = { { 0, 0 } };

    // The current starting index for the offset index data
    uint32_t _startIndex = 0;

    // The starting index for spectators
    uint32_t _spectateStartIndex = 0;

    // Mapping: player -> index offset -> frame -> input
    std::array< InputsContainer< uint16_t >, 2 > _inputs;

    // Mapping: index offset -> RngState (can be null)
    std::vector< MsgPtr > _rngStates;
    std::vector< RngState* > _roundRngStates;

    // Mapping: index offset -> retry menu index (invalid is -1)
    std::vector< int8_t > _retryMenuIndicies;

    // The local player, ie the one where setInput is called each frame locally
    uint8_t _localPlayer = 1;

    // The remote player, ie the one where setInputs gets called for each input
    // message
    uint8_t _remotePlayer = 2;

    // Exported
    bool exported = false;

    // Separate delays for p1/p2
    bool splitDelay = true;

    // Delay demo inputs until reset is done
    int demoCountdown = 60;
    int exitCountdown = 0;

    // Get the input for the specific NetplayState
    uint16_t getPreInitialInput( uint8_t player );
    uint16_t getInitialInput( uint8_t player );
    uint16_t getAutoCharaSelectInput( uint8_t player );
    uint16_t getCharaSelectInput( uint8_t player );
    uint16_t getSkippableInput( uint8_t player );
    uint16_t getDemoInput( uint8_t player );
    uint16_t getInGameInput( uint8_t player );
    uint16_t getRetryMenuInput( uint8_t player );
    uint16_t getReplayMenuInput( uint8_t player );

    // Get the input needed to navigate the menu
    uint16_t getMenuNavInput();

    // Detect if a key has been pressed / held by either player in the input
    // history. The start and end indicies begin from the current frame and
    // count backwards.
    bool hasUpDownInHistory( uint8_t player,
                             uint32_t start,
                             uint32_t end ) const;
    bool hasButtonInHistory( uint8_t player,
                             uint16_t button,
                             uint32_t start,
                             uint32_t end ) const;
    bool heldButtonInHistory( uint8_t player,
                              uint16_t button,
                              uint32_t start,
                              uint32_t end ) const;

    // Get the buffered preserveStartIndex
    uint32_t getBufferedPreserveStartIndex() const;

    std::string sanitizePlayerName( std::string name );
    void findAndReplaceAll( std::string& data,
                            std::string toSearch,
                            std::string replaceStr );
    std::string getISOTime();
};

extern NetplayManager* netManPtr;
