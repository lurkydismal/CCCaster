#!/bin/sh
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
    clang-format --style=file \
    -i \
    addon_callbacks/src/*.cpp addon_callbacks/include/*.hpp \