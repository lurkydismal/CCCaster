#pragma once

// Current game mode address
#define CC_GAME_MODE_ADDR ( 0x54EEE8 )

typedef enum {
    STARTUP = 65535,
    OPENING = 3,
    TITLE = 2,
    MAIN = 25,
    CHARACTER_SELECT = 20,
    LOADING = 8,
    IN_MATCH = 1,
    RETRY = 5,
    LOADING_DEMO = 13,
    REPLAY = 26,
    HIGH_SCORES = 11
} gameMode_t;
