#!/bin/bash
ccache wineg++ \
    -m32 -shared \
    -std=gnu++26 \
    -pipe -fuse-ld=mold -march=native \
    -ffunction-sections -fdata-sections \
    -fPIC -fopenmp-simd \
    -fno-ident -fno-short-enums \
    -Wall -Wextra \
    -O3 -ffast-math -funroll-loops \
    -fno-asynchronous-unwind-tables \
    -fno-rtti -fno-exceptions \
    -fno-threadsafe-statics \
    -fno-unwind-tables \
    -fPIC -Wl,-O1 -Wl,--gc-sections \
    -s -Wl,--no-eh-frame-hdr \
    ./*.cpp -o wrapper.dll
