#pragma once

extern char*** g_settings;

#define DEFAULT_SETTINGS \
    ( "[keyboard]\n"     \
      "8 = Up\n"         \
      "6 = Right\n"      \
      "2 = Down\n"       \
      "4 = Left\n"       \
      "A = Z\n"          \
      "B = X\n"          \
      "C = C\n"          \
      "D = V\n"          \
      "E = D\n"          \
      "AB = S\n"         \
      "FN1 = ]\n"        \
      "FN2 = R\n"        \
      "START = T\n" )

#define DEFAULT_ELEMENTS_ORDER "text,rectangle"

#define DEFAULT_ELEMENTS_SETTINGS \
    ( "[text]\n"                  \
      "text = TEST\n"             \
      "x = 200\n"                 \
      "y = 200\n"                 \
      "height = 50\n"             \
      "width = 50\n"              \
      "alpha = 255\n"             \
      "shade_first = 1\n"         \
      "layer = 1\n"               \
      "[rectangle]\n"             \
      "x = 200\n"                 \
      "y = 200\n"                 \
      "height = 150\n"            \
      "width = 150\n"             \
      "a_red = 255\n"             \
      "d_green = 255\n"           \
      "alpha = 255\n"             \
      "layer = 1\n" )
