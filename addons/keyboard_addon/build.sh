#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
mkdir -p ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/keyboard && \
i686-w64-mingw32-g++-win32 -shared -s \
    -Ofast \
    -fpermissive \
    -I ../../json_file/include \
    -I ../../states_dll/_useCallback/include \
    -I ../overlay_addon/imgui/include \
    -I ../overlay_addon/native/include \
    -I addon_callbacks/include \
    -I button_mappings/include \
    -I controls_parse/include \
    -I direction_mappings/include \
    -I hotkey_mappings/include \
    -I ../ \
    -I ../../ \
    src/*.cpp \
    addon_callbacks/src/*.cpp \
    button_mappings/src/*.cpp \
    direction_mappings/src/*.cpp \
    hotkey_mappings/src/*.cpp \
    ../../json_file/src/*.cpp \
    ../../states_dll/_useCallback/src/_useCallback.cpp \
    -std=c++11 \
    -o keyboard.dll && \
    mv -f keyboard.dll \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/keyboard/. && \
    cp -f info.hjson \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/addons/keyboard/. && \
    cp -f controls_backup.json \
    ../../../MBAACC\ -\ Community\ Edition/MBAACC/. && \
    clang-format-15 --style=file \
    -i \
    src/*.cpp \
    addon_callbacks/include/*.hpp addon_callbacks/src/*.cpp \
    button_mappings/include/*.hpp button_mappings/src/*.cpp \
    controls_parse/include/*.hpp \
    direction_mappings/include/*.hpp direction_mappings/src/*.cpp \
    hotkey_mappings/src/*.cpp hotkey_mappings/include/*.hpp
