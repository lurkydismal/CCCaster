#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
mkdir -p ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/caster && \
i686-w64-mingw32-g++-win32 -shared -s \
    -Ofast \
    -I ../../dxsdk \
    -I ../../states_dll/_useCallback/include \
    -I addon_callbacks/include \
    -I applyInput/include \
    -I patch_callbacks/include \
    -I patch_t/include \
    -I ../ \
    src/*.cpp \
    addon_callbacks/src/*.cpp \
    applyInput/src/*.cpp \
    patch_callbacks/src/*.cpp \
    patch_t/src/*.cpp \
    ../../states_dll/_useCallback/src/_useCallback.cpp \
    -std=c++11 \
    -o caster.dll && \
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
