#!/bin/bash
# -static-libgcc -static-libstdc++
source ../config.sh && \
clear && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f d3d9.dll.so \
    "${MBAA_DIR}/d3d9.dll" && \
    clang-format-18 --style=file \
    -i \
    src/*.cpp src/*.h \
    ../minhook/include/*.h
