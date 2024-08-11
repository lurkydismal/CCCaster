#!/bin/sh
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
    clang-format --style=file \
    -i \
    addon_callbacks/src/*.c addon_callbacks/include/*.h \
    controls_parse/include/*.h
