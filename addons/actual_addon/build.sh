#!/bin/bash
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
    src/*.cpp \
    addon_callbacks/src/*.cpp \
    patch_callbacks/src/*.cpp patch_callbacks/include/*.hpp \
    patch_t/src/*.cpp patch_t/include/*.hpp
