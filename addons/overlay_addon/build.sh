#!/bin/sh
# -static-libgcc -static-libstdc++
source ../../config.sh && \
clear && \
mkdir -p "${ADDONS_DIR}/overlay" && \
    i686-w64-mingw32-gcc -Ofast -march=native -m32 -s -c -o native/src/CallDraw.o native/src/CallDraw.s && \
    i686-w64-mingw32-ar rcs -o native/src/libCallDraw.a native/src/CallDraw.o && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f overlay.dll.so \
    "${ADDONS_DIR}/overlay/overlay.dll" && \
    cp -f info.hjson \
    "${ADDONS_DIR}/overlay/." && \
    clang-format --style=file \
    -i \
    addon_callbacks/src/*.c addon_callbacks/include/*.h \
    native/src/*.c native/include/*.h
