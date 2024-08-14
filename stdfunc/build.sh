#!/bin/sh
# -static-libgcc -static-libstdc++
clear && \
    clang-format --style=file \
    -i \
    src/*.c include/*.h
