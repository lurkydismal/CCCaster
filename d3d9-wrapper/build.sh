#!/bin/sh
# -static-libgcc -static-libstdc++
source ../config.sh && \
clear && \
    make clean && \
    make -j $(( `nproc` - 1 )) && \
    mv -f d3d9.dll.so \
    "${MBAA_DIR}/d3d9.dll" && \
    clang-format --style=file \
    -i \
    src/*.cpp src/*.h \
    D3DPERF_BeginEvent/src/*.c D3DPERF_BeginEvent/include/*.h \
    D3DPERF_EndEvent/src/*.c D3DPERF_EndEvent/include/*.h \
    D3DPERF_GetStatus/src/*.c D3DPERF_GetStatus/include/*.h \
    D3DPERF_QueryRepeatFrame/src/*.c D3DPERF_QueryRepeatFrame/include/*.h \
    D3DPERF_SetMarker/src/*.c D3DPERF_SetMarker/include/*.h \
    D3DPERF_SetOptions/src/*.c D3DPERF_SetOptions/include/*.h \
    D3DPERF_SetRegion/src/*.c D3DPERF_SetRegion/include/*.h \
    DebugSetLevel/src/*.c DebugSetLevel/include/*.h \
    DebugSetMute/src/*.c DebugSetMute/include/*.h \
    Direct3D9EnableMaximizedWindowedModeShim/src/*.c Direct3D9EnableMaximizedWindowedModeShim/include/*.h \
    Direct3DCreate9/src/*.cpp Direct3DCreate9/include/*.h \
    Direct3DCreate9Ex/src/*.cpp Direct3DCreate9Ex/include/*.h \
    Direct3DShaderValidatorCreate9/src/*.c Direct3DShaderValidatorCreate9/include/*.h \
    PSGPError/src/*.c PSGPError/include/*.h \
    PSGPSampleTexture/src/*.c PSGPSampleTexture/include/*.h
