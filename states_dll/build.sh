#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f states.dll.so \
    ../../MBAACC\ -\ Community\ Edition/MBAACC/states.dll && \
    clang-format-15 --style=file \
    -i \
    src/*.cpp \
    _useCallback/src/*.cpp _useCallback/include/*.hpp
