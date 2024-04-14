#!/bin/bash
# -static-libgcc -static-libstdc++
clear && \
i686-w64-mingw32-g++-win32 -c -s \
    -Ofast \
    -I include \
    -I ../ \
    src/*.cpp \
    -std=c++11 \
    -o json_file.obj && \
    clang-format-15 --style=file \
    -i \
    src/*.cpp include/*.hpp
