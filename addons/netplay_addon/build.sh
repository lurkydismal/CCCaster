#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
mkdir -p ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/netplay && \
i686-w64-mingw32-g++-win32 -shared -s \
    -Ofast \
    -fpermissive \
    -I ../../json_file/include \
    -I ../../states_dll/_useCallback/include \
    -I ../overlay_addon/imgui/include \
    -I ../actual_addon/applyInput/include \
    -I addon_callbacks/include \
    -I hotkey_mappings/include \
    -I ../ \
    -I ../../ \
    src/*.cpp \
    addon_callbacks/src/*.cpp \
    hotkey_mappings/src/*.cpp \
    ../actual_addon/applyInput/src/*.cpp \
    ../../states_dll/_useCallback/src/_useCallback.cpp \
    -lws2_32 \
    -lmswsock \
    -std=c++11 \
    -o netplay.dll && \
    mv -f netplay.dll \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/netplay/. && \
    cp -f info.hjson \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/netplay/. && \
    clang-format-15 --style=file \
    -i \
    src/*.cpp \
    addon_callbacks/src/*.cpp addon_callbacks/include/*.hpp \
    hotkey_mappings/src/*.cpp hotkey_mappings/include/*.hpp
