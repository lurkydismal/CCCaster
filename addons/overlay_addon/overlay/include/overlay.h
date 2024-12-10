#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "native.h"

#define OVERLAY_HOTKEY_PART1 \
    "overlay_toggle_key"     \
    "_"

#define DEFAULT_ELEMENT_PARAMETERS \
    { UNKNOWN,                     \
      { 0, 0 },                    \
      { 0, 0 },                    \
      { 0, 0 },                    \
      { 0, 0 },                    \
      { 0, 0, 0, 0 },              \
      { 0, 0, 0, 0 },              \
      { 0, 0, 0, 0 },              \
      { 0, 0, 0, 0 },              \
      "",                          \
      { 0, 0 },                    \
      FONT0,                       \
      0,                           \
      0,                           \
      false,                       \
      false }

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
} color_t;

typedef struct {
    int32_t x;
    int32_t y;
} coordinates_t;

struct size {
    uint32_t width;
    uint32_t height;
};

typedef struct {
    uint32_t first;
    uint32_t second;
} shade_t;

enum elementType { RECTANGLE, TEXT, SPRITE, BIND, UNKNOWN, TOTAL };

extern char* g_elementTypesAsString[ ( enum elementType )TOTAL ];

typedef struct {
    enum elementType type;
    coordinates_t coordinates;
    coordinates_t textureCoordinates;
    struct size size;
    struct size textureSize;
    color_t a;
    color_t b;
    color_t c;
    color_t d;
    char* text;
    shade_t shade;
    uintptr_t* fontAddress;
    uint32_t letterSpacing;
    uint32_t layer;
    bool isActive;
    bool canActive;
} element_t;

extern element_t*** g_overlaysToRender;
extern char** g_overlayHotkeys;
extern char** g_overlayNames;

uint16_t overlayRegister( const char* _overlayName,
                          const char* const* _elementsOrder,
                          const char* _elementsDefaultSettings,
                          const uintptr_t* _elementsCallbackVariableReferences,
                          const char* _overlayDefaultHotkey );

uint16_t drawElement( const element_t* _element );
uint16_t interactElement( const element_t* _element,
                          const char*** _activeMappedKeys,
                          const char*** _activeKeys );

uint32_t getColorForRectangle( color_t _color );
