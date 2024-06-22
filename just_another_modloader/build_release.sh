#!/bin/sh
source ../config.sh && \
cargo build --target=i686-pc-windows-gnu --release && \
mv -f target/i686-pc-windows-gnu/release/just_another_modloader.dll \
"${MBAA_DIR}/just_another_modloader.dll"
