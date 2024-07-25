#!/bin/sh
# -static-libgcc -static-libstdc++
source ../../config.sh && \
clear && \
mkdir -p "${ADDONS_DIR}/actual_addon" && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f actual_addon.dll.so \
    "${ADDONS_DIR}/actual_addon/actual_addon.dll" && \
    cp -f info.hjson \
    "${ADDONS_DIR}/actual_addon/." && \
    clang-format --style=file \
    -i \
    src/*.c \
    addon_callbacks/src/*.c \
    patch_callbacks/src/*.c patch_callbacks/include/*.h \
    patch_t/src/*.c patch_t/include/*.h
