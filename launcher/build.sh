#!/bin/bash
ccache \
    clang++ --target=i686-w64-windows-gnu \
    -pipe -march=native -ffunction-sections -fdata-sections -fPIC -fopenmp-simd -fno-ident -fno-short-enums -Wall -Wextra -Wno-gcc-compat -Wno-incompatible-pointer-types-discards-qualifiers \
    -O3 -ffast-math -funroll-loops -fno-asynchronous-unwind-tables \
    -std=gnu++26 -fno-rtti -fno-exceptions -fno-threadsafe-statics -Wno-enum-enum-conversion -Wno-c99-designator -Wno-gnu-string-literal-operator-template \
    -fno-unwind-tables \
    -fPIC -Wl,-O1 -Wl,--gc-sections \
    -s \
    launcher.cpp -o launcher.exe
