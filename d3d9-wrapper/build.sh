#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
ccache i686-w64-mingw32-g++-win32 -shared -s -m32 \
    -Ofast \
    -fvisibility=hidden \
    -I ../states_dll/_useCallback/include \
    -I ../dxsdk \
    src/*.cpp \
    ../states_dll/_useCallback/src/_useCallback.cpp \
    -luuid \
    -std=c++2a \
    -o d3d9.dll && \
    mv -f d3d9.dll \
    ../../MBAACC\ -\ Community\ Edition/MBAACC/. && \
    clang-format-15 --style=file \
    -i \
    src/*.cpp src/*.h
