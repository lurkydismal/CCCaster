#!/bin/bash
# -static-libgcc -static-libstdc++
source ../../config.sh && \
clear && \
mkdir -p "${ADDONS_DIR}/netplay" && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f netplay.dll.so \
    "${ADDONS_DIR}/netplay/netplay.dll" && \
    cp -f info.hjson \
    "${ADDONS_DIR}/netplay/." && \
    clang-format-18 --style=file \
    -i \
    src/*.cpp \
    addon_callbacks/src/*.cpp addon_callbacks/include/*.hpp \
    hotkey_mappings/src/*.cpp hotkey_mappings/include/*.hpp
