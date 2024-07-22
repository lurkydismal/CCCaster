#!/bin/sh
# -static-libgcc -static-libstdc++
source ../../config.sh && \
clear && \
mkdir -p "${ADDONS_DIR}/joystick" && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f joystick.dll.so \
    "${ADDONS_DIR}/joystick/joystick.dll" && \
    cp -f info.hjson \
    "${ADDONS_DIR}/joystick/." && \
    cp -f controls_backup.json \
    "${MBAA_DIR}/." && \
    clang-format --style=file \
    -i \
    src/*.cpp \
    addon_callbacks/src/*.cpp addon_callbacks/include/*.hpp \
    controls_parse/src/controls_parse.c controls_parse/include/*.hpp
