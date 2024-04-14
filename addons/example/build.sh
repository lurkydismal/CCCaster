#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
mkdir -p ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/example && \
i686-w64-mingw32-g++-win32 -shared -s \
    example.cpp \
    -Ofast \
    -std=c++11 \
    -o example.dll && \
    mv -f example.dll \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/example/. && \
    cp -f info.hjson \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/example/. && \
    clang-format-15 --style=file \
    -i \
    example.cpp
