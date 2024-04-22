#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f d3d9.dll.so \
    ../../MBAACC\ -\ Community\ Edition/MBAACC/d3d9.dll && \
    clang-format-15 --style=file \
    -i \
    src/*.cpp src/*.h \
    ../minhook/include/*.h
