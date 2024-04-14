#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
mkdir -p ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/overlay && \
ccache i686-w64-mingw32-g++-win32 -shared -s \
    -Ofast \
    -I ../../states_dll/_useCallback/include \
    -I addon_callbacks/include \
    -I native/include \
    -I ../../dxsdk \
    -I ../../imgui \
    -I ../../imgui/backends \
    -I ../../imgui/misc/cpp \
    -I ../../ \
    src/*.cpp \
    addon_callbacks/src/*.cpp \
    imgui/src/*.cpp \
    native/src/*.cpp \
    native/src/*.s \
    ../../states_dll/_useCallback/src/_useCallback.cpp \
    ../../imgui/*.cpp \
    ../../imgui/backends/*.cpp \
    ../../imgui/misc/cpp/*.cpp \
    /usr/i686-w64-mingw32/lib/libgdi32.a \
    /usr/i686-w64-mingw32/lib/libdwmapi.a \
    -std=c++11 \
    -o overlay.dll && \
    mv -f overlay.dll \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/overlay/. && \
    cp -f info.hjson \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/overlay/. && \
    clang-format-15 --style=file \
    -i \
    src/*.cpp \
    addon_callbacks/include/*.hpp addon_callbacks/src/*.cpp \
    imgui/include/*.hpp imgui/src/*.cpp \
    native/include/*.hpp native/src/*.cpp
