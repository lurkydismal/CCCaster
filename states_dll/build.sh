#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
i686-w64-mingw32-g++-win32 -shared -s \
    -Ofast \
    src/main.cpp \
    -std=c++11 \
    -o states.dll && \
    mv -f states.dll \
    ../../MBAACC\ -\ Community\ Edition/MBAACC/states.dll && \
    clang-format-15 --style=file \
    -i \
    src/*.cpp \
    _useCallback/src/*.cpp _useCallback/include/*.hpp
