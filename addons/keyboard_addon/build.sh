#!/bin/bash
# -static-libgcc -static-libstdc++
source ../../config.sh && \
clear && \
mkdir -p "${ADDONS_DIR}/keyboard" && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f keyboard.dll.so \
    "${ADDONS_DIR}/keyboard/keyboard.dll" && \
    cp -f info.hjson \
    "${ADDONS_DIR}/keyboard/." && \
    cp -f controls_backup.json \
    "${MBAA_DIR}/." && \
    clang-format --style=file \
    -i \
    src/*.cpp \
    addon_callbacks/include/*.hpp addon_callbacks/src/*.cpp \
    controls_parse/include/*.hpp \
