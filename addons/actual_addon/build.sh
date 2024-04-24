#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
mkdir -p ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/actual_addon && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f actual_addon.dll.so \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/actual_addon/actual_addon.dll && \
    cp -f info.hjson \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/actual_addon/. && \
    clang-format-15 --style=file \
    -i \
    src/*.cpp \
    addon_callbacks/src/*.cpp addon_callbacks/include/*.hpp \
    applyInput/src/*.cpp applyInput/include/*.hpp \
    patch_callbacks/src/*.cpp patch_callbacks/include/*.hpp \
    patch_t/src/*.cpp patch_t/include/*.hpp
