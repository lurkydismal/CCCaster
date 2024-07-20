#!/bin/sh
# -static-libgcc -static-libstdc++
source ../config.sh && \
clear && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f states.dll.so \
    "${MBAA_DIR}/states.dll" && \
    clang-format --style=file \
    -i \
    src/*.cpp \
    _useCallback/src/*.c _useCallback/include/*.h
