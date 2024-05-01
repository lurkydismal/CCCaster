#!/bin/bash
# -static-libgcc -static-libstdc++
source ../config.sh && \
clear && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f states.dll.so \
    "${MBAA_DIR}/states.dll" && \
    clang-format-18 --style=file \
    -i \
    src/*.cpp \
    _useCallback/src/*.cpp _useCallback/include/*.hpp
