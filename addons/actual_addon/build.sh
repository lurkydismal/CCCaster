#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
    make clean &&
    make -j 11 &&
    mv -f caster.dll \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/caster/. && \
    cp -f info.hjson \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/caster/. && \
    clang-format-15 --style=file \
    -i \
    src/*.cpp \
    addon_callbacks/src/*.cpp addon_callbacks/include/*.hpp \
    applyInput/src/*.cpp applyInput/include/*.hpp \
    patch_callbacks/src/*.cpp patch_callbacks/include/*.hpp \
    patch_t/src/*.cpp patch_t/include/*.hpp
