#!/bin/bash
mkdir build || \
rm build/*

cargo build --target=i686-pc-windows-gnu

cp target/i686-pc-windows-gnu/debug/*.dll build/addon_loader.dll && \
    mv -f build/addon_loader.dll \
    ../../MBAACC\ -\ Community\ Edition/MBAACC/addon_loader.dll
