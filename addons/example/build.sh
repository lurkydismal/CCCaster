#!/bin/bash
# -static-libgcc -static-libstdc++
source ../../config.sh && \
clear && \
mkdir -p "${ADDONS_DIR}/example" && \
    make clean && \
    make -j $( ( `nproc` - 1 )) && \
    mv -f example.dll.so \
    "${ADDONS_DIR}/example/example.dll" && \
    cp -f info.hjson \
    "${ADDONS_DIR}/example/." && \
    clang-format --style=file \
    -i \
    example.cpp
