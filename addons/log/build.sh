#!/bin/sh
# -static-libgcc -static-libstdc++
source ../../config.sh && \
clear && \
mkdir -p "${ADDONS_DIR}/log" && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f log.dll.so \
    "${ADDONS_DIR}/log/log.dll" && \
    cp -f info.hjson \
    "${ADDONS_DIR}/log/." && \
    clang-format --style=file \
    -i \
    src/*.c \
    log/include/*.h log/src/*.c \
    addon_callbacks/src/*.c
