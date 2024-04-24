#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
mkdir -p ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/example && \
    make clean && \
    make -j $( ( `nproc` - 1 )) && \
    mv -f example.dll.so \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/example/example.dll && \
    cp -f info.hjson \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/example/. && \
    clang-format-15 --style=file \
    -i \
    example.cpp
