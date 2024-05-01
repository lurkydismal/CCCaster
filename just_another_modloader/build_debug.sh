#!/bin/bash
source ../config.sh && \
cargo build --target=i686-pc-windows-gnu && \
mv -f target/i686-pc-windows-gnu/debug/just_another_modloader.dll \
"${MBAA_DIR}/just_another_modloader.dll"
